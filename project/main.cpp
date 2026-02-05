#include<Windows.h>
#include<cstdint>
#include<string>
#include<filesystem>
#include<fstream>
#include<chrono>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<dbghelp.h>
#include<strsafe.h>
#include<dxgidebug.h>
#include<dxcapi.h>
#include"Calculation.h"
#include"externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include"externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#include"externals/DirectXTex/DirectXTex.h"
#include"externals/DirectXTex/d3dx12.h"
#include<vector>
#include <numbers>
#include<fstream>
#include<sstream>
#include<wrl.h> // ComPtr を使用するために必要
#include"Input.h"
#include"WinApp.h"
#include"DirectXCommon.h"
#include"Logger.h"
#include"StringUtility.h"
#include"SpriteCommon.h"
#include"Sprite.h"
#include"Structs.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

// ComPtr を簡単に使うために名前空間をusing
using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;


// Transform変数を作る (Sphere用)
Transform transform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
Transform cameraTransform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-5.0f} };

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dmupFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processId(このexeのId)とクラッシュ(例外)の発生したtheradIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInfomation{ 0 };
	minidumpInfomation.ThreadId = threadId;
	minidumpInfomation.ExceptionPointers = exception;
	minidumpInfomation.ClientPointers = TRUE;
	// Dumpを出力。MiniDumpNormalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dmupFileHandle, MiniDumpNormal, &minidumpInfomation, nullptr, nullptr);
	// 他に関連付けられているSEH例外ハンドラがあれば実行。通常プロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}



DirectX::ScratchImage LoadTexture(const std::string& filePath) {
	// テクスチャの読み込み
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipChain{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipChain);
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータデータを返す
	return mipChain;
}

ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
	// テクスチャのリソースを作成
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// 利用するHeepの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;


	// Resourceの生成
	ComPtr <ID3D12Resource> resource = nullptr; 
	HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

[[nodiscard]]
ComPtr<ID3D12Resource> UploadTextureData(ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, DirectXCommon* dxCommon) {
	// Mate情報を取得
	std::vector<D3D12_SUBRESOURCE_DATA>subresorce;
	DirectX::PrepareUpload(dxCommon->GetDevice(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresorce); // 変更点: device.Get() を使用
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresorce.size()));
	ComPtr <ID3D12Resource> intermediateResource = dxCommon->CreatBufferResource(intermediateSize); // 変更点: CreatBufferResouce に device.Get() を渡す
	UpdateSubresources(dxCommon->GetCommandList(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresorce.size()), subresorce.data()); // 変更点: texture.Get() と intermediateResource.Get() を使用

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	dxCommon->GetCommandList()->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descrptorSize, uint32_t index) {
	// ディスクリプタヒープの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// インデックス分だけオフセットを加える
	handleCPU.ptr += static_cast<SIZE_T>(descrptorSize) * index;
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descrptorSize, uint32_t index) {
	// ディスクリプタヒープの先頭を取得
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// インデックス分だけオフセットを加える
	handleGPU.ptr += static_cast<SIZE_T>(descrptorSize) * index;
	return handleGPU;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::fstream file(directoryPath + "/" + filename);
	assert(file.is_open());
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;

			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;
	std::fstream file(directoryPath + "/" + filename);
	assert(file.is_open());
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			position.x *= -1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {
			VertexData triangle[3];
			for (int32_t facevertex = 0; facevertex < 3; ++facevertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				std::istringstream v(vertexDefinition);
				uint32_t elementIndice[3];
				for (uint32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/');
					elementIndice[element] = std::stoi(index);
				}
				Vector4 position = positions[elementIndice[0] - 1];
				Vector2 texcoord = texcoords[elementIndice[1] - 1];
				Vector3 normal = normals[elementIndice[2] - 1];
				triangle[facevertex] = { position, texcoord, normal };
			}
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;

			// マテリアルファイルを読み込む
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}

struct D3DResourceLeakCheker {
	~D3DResourceLeakCheker() {
		ComPtr <IDXGIDebug1> debug; // 変更点: ComPtr を使用
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakCheker leakChecker; // D3Dリソースリークチェック用のオブジェクト
	CoInitializeEx(0, COINIT_MULTITHREADED);
	SetUnhandledExceptionFilter(ExportDump);

	Calculation calculation;
	Input* input = nullptr;
	WinApp* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;
	SpriteCommon* spriteCommon = nullptr;
	Sprite* sprite = new Sprite();

	// Inputの初期化
	input = new Input();
	// WinAppの初期化
	winApp = new WinApp();
	// DirectXCommonの初期化
	dxCommon = new DirectXCommon();
	// SpriteCommonの初期化
	spriteCommon = new SpriteCommon();

	// logsフォルダを作成
	std::filesystem::create_directories("logs");
	//　現在時刻を取得（UTC時刻）
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// ログファイルの名前にコンマ何秒はいらないため削っておく
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	// 日本時間(PCの確定時間)に変換
	std::chrono::zoned_time localTime(std::chrono::current_zone(), nowSeconds);
	// formatを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	// 時刻を使ってファイルを決定
	std::string logFilePath = std::string("logs/") + dateString + ".log";
	//ファイルを使って書き込み準備
	std::ofstream logstream(logFilePath);

	/*uint32_t* p = nullptr;
	*p = 100;*/

	winApp->Initialize();
	input->Initialize(winApp);
	dxCommon->Initialize(winApp);
	dxCommon->ImGuiInitialize();
	spriteCommon->Initialize(dxCommon);
	sprite->Initialize(spriteCommon,1);
	sprite->Initialize(spriteCommon, 2);

	HRESULT hr = S_OK; // これを追加

	// Sphere用のWVPリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = dxCommon->CreatBufferResource(sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpData = nullptr;
	// 書き込むためのアドレス取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	// 単位行列を書き込んでおく
	wvpData->World = calculation.MakeIdentity4x4();
	wvpData->WVP = calculation.MakeIdentity4x4();
	wvpResource->Unmap(0, nullptr);

	constexpr uint32_t kSubdivision = 16; // 分割数

	constexpr uint32_t kVertexCount = kSubdivision * kSubdivision * 6;

	constexpr uint32_t kIndexCount = kVertexCount;

	//モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");

	// Sphere用の頂点リソース 
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = dxCommon->CreatBufferResource(sizeof(VertexData) * modelData.vertices.size());

	// Sphere用の頂点バッファビューを作成する 
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// Sphere用のインデックスリソースとビュー
	Microsoft::WRL::ComPtr < ID3D12Resource> indexBuffer = dxCommon->CreatBufferResource(sizeof(uint32_t) * kIndexCount);
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32ビット符号なし整数

	// Sphere用の頂点リソースにデータを書き込む 
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
	vertexResource->Unmap(0, nullptr);

	// Sphere用のインデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// Sphere用のマテリアルリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = dxCommon->CreatBufferResource(sizeof(Material));
	Material* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;
	
	
	
	

	// DirectionalLight用のリソース
	Microsoft::WRL::ComPtr < ID3D12Resource> directionalLightResource = dxCommon->CreatBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f, 1.0f, 1.0f }; // 光の色
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f }; // 光の方向
	directionalLightData->intensity = 1.0f; // 光の強度
	directionalLightData->direction = calculation.Normalize(directionalLightData->direction); // 光の方向を正規化

	
	
	// --- テクスチャのロードとアップロード、SRVの作成 ---
	// テクスチャ1のロードとアップロード
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = CreateTextureResource(dxCommon->GetDevice(), metadata);
	Microsoft::WRL::ComPtr < ID3D12Resource> textureUploadHeap = UploadTextureData(textureResource.Get(), mipImages, dxCommon);

	// テクスチャ2のロードとアップロード
	DirectX::ScratchImage mipImage2 = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata metadata2 = mipImage2.GetMetadata();
	// 変更点: CreateTextureResource の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = CreateTextureResource(dxCommon->GetDevice(), metadata2); // metadata2を使用
	// 変更点: UploadTextureData の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr <ID3D12Resource> textureUploadHeap2 = UploadTextureData(textureResource2.Get(), mipImage2, dxCommon);

	// コマンドリストを閉じて実行、アップロード完了を待機
	hr = dxCommon->GetCommandList()->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandListsForUpload[] = { dxCommon->GetCommandList() };
	dxCommon->GetCommandQueue()->ExecuteCommandLists(1, commandListsForUpload);
	dxCommon->WaitForGpu();

	// コマンドリストとアロケーターをリセットして、メインループで再度使用できるようにする
	hr = dxCommon->GetCommandAllocator()->Reset(); assert(SUCCEEDED(hr));
	hr = dxCommon->GetCommandList()->Reset(dxCommon->GetCommandAllocator(), nullptr); assert(SUCCEEDED(hr));

	// SRVの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format; // テクスチャのフォーマット
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // テクスチャの種類
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); // MipMapの数

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// SRVを作成するDescriotorHeapの場所を決める (ImGuiが0を使うので、1から開始)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandelCPU = dxCommon->GetSrvCPUHandle(1); // Index 1
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandelCPU2 = dxCommon->GetSrvCPUHandle(2); // Index 2
	// SRVの生成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandelCPU);
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandelCPU2); // srvDesc2を使用

	
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度ステンシルのフォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 深度ステンシルの種類

	dxCommon->GetDevice()->CreateDepthStencilView(dxCommon->GetDepthStencilResource(), &dsvDesc, dxCommon->GetDsvHandle());

	
	bool useMonsterBall = true;
	
	// --- メインループ ---
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {
		//メッセージがあるか確認
		if (winApp->ProcessMassage()) {
			break;
		}
			input->Update(); // キー入力の更新

			dxCommon->ImGuiPreDraw();
			sprite->Update();
			// Sphereの回転
			transform.rotate.y += 0.01f;
			Matrix4x4 woldMatrix = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

			Matrix4x4 cameraMatrix = calculation.MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = calculation.Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = calculation.MakePerspectiveFovMatrix(0.45f, float(WinApp::KclientWidth) / float(WinApp::KclientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = calculation.Multiply(woldMatrix, calculation.Multiply(viewMatrix, projectionMatrix));
			wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
			wvpData->WVP = worldViewProjectionMatrix; // SphereのWVPを更新
			wvpData->World = woldMatrix; // SphereのWorldを更新
			wvpResource->Unmap(0, nullptr);
			
			// 開発用UIの処理
			ImGui::ShowDemoWindow();
			// カラー
			ImGui::Begin("MaterialColor");
			ImGui::ColorEdit4("Color", &materialData->color.x);
			// 光の方向 (X, Y, Z 成分をスライダーで調整)
			ImGui::SliderFloat3("Direction", &directionalLightData->direction.x, -1.0f, 1.0f);
			directionalLightData->direction = calculation.Normalize(directionalLightData->direction); // 光の方向を正規化
			// 光の強度 (スライダーで調整)
			ImGui::SliderFloat("Intensity", &directionalLightData->intensity, 0.0f, 5.0f); // 0.0から5.0の範囲で調整可能
			// 光の色 (ColorPickerで調整)
			ImGui::ColorEdit4("Light Color", &directionalLightData->color.x);
			ImGui::Checkbox("Use Monster Ball Texture", &useMonsterBall); // ImGuiで切り替えできるように追加

			ImGui::End();

			sprite->ImGui();

			dxCommon->PreDraw();

			// 描画用のDescriptorHeapを設定 (SRV/CBV/UAV用)
			ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon->GetSrvHeap() };
			dxCommon->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

			//// RootSignatureとPipelineStateを設定 (Sphere用)
			//dxCommon->GetCommandList()->SetGraphicsRootSignature(dxCommon->GetRootSignature());
			//dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
			//dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView); // Sphereの頂点バッファ
			//dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView); // Sphereのインデックスバッファ
			//dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 形状を設定

			//// Sphere用のリソースをバインド
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // Sphereのマテリアル
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress()); // SphereのWVP
			//dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress()); // DirectionalLight
			//dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandelGPU2 : textureSrvHandelGPU); // Sphereのテクスチャ

			//// Sphereを描画
			//dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			spriteCommon->PreDraw();

			sprite->Draw();
			dxCommon->ImGuiPostDraw();
			dxCommon->PostDraw();
	}
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているかどうか、どうにもできない場合が多いのでassertにしておく
	Log("Hello, DirectX!\n");
	Log(ConvertString(std::format(L"windth\n", WinApp::KclientWidth)));

	CloseHandle(dxCommon->GetFenceEvent());
	dxCommon->ImGuiFinalize();
	winApp->Finalize();

	delete input;
	delete winApp;
	delete dxCommon;
	delete spriteCommon;
	delete sprite;

	return 0;
}

// マージ確認用コメント