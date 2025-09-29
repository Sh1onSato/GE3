#include<Windows.h>
#include<cstdint>
#include<string>
#include<format>
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

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

// ComPtr を簡単に使うために名前空間をusing
using namespace Microsoft::WRL;

struct Vector2 {
	float x;
	float y;
};

struct Vector3 {
	float x, y, z;
};

struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

struct Transform {
	Calculation::Vector3 scale,
		rotate,
		translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Calculation::Matrix4x4 uvTransform;
};

struct TransformationMatrix {
	Calculation::Matrix4x4 WVP;
	Calculation::Matrix4x4 World;

};

struct DirectionalLight {
	Vector4 color;
	Calculation::Vector3 direction;
	float intensity;
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

// Transform変数を作る (Sphere用)
Transform transform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
Transform cameraTransform{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,-5.0f} };

const int32_t KclientWidth = 1200;
const int32_t KclientHeight = 720;

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

void Log(std::ostream& os, const std::string& message) {
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
		return true;
	}
	switch (msg)
	{
		//ウィンドウが破壊された
	case WM_DESTROY:
	{
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);

		return 0;
	}
	}
	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

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

// 変更点: 戻り値を ComPtr<IDxcBlob> に変更
ComPtr<IDxcBlob> CompileShader(
	// CompileするShaderのファイルへのパス
	const std::wstring& filePath,
	// Compileに使用するProfilae
	const wchar_t* profile,
	// 初期化で生成したものを3つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler,
	std::ostream& os)
{
	// ここからシェーダーをコンパイルする旨をログに出す
	Log(os, ConvertString(std::format(L"Begin CompileShader,path:{}\n", filePath, profile)));
	// hlslファイルを読む
	ComPtr <IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_ACP;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main", // エントリーポイント
		L"-T", profile, // プロファイル
		L"-Zi",L"-Qembed_debug", // デバッグ情報を埋め込む
		L"-Od", // 最適化しない
		L"-Zpr",
	};
	// 実際にShaderをCompileする
	ComPtr <IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments, _countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	// 警告・エラーが出たらログに出して止める
	ComPtr<IDxcBlobUtf8> shaderError = nullptr; // 変更点: ComPtr を使用
	shaderResult->GetOutput(
		DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		// エラーがあった場合は、エラーメッセージをログに出す
		Log(shaderError->GetStringPointer());
		assert(false);
	}
	// コンパイル結果を取得する
	ComPtr<IDxcBlob> shaderBlob = nullptr; // 変更点: ComPtr を使用
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功したログを出す
	Log(os, ConvertString(std::format(L"Compile Success,path:{}, profile:{}\n", filePath, profile)));

	// 実行用のバイナリを返却
	return shaderBlob;
};

// 変更点: 戻り値を ComPtr<ID3D12Resource> に変更
ComPtr<ID3D12Resource> CreatBufferResouce(ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // ヒープの種類
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	// バッファの場合はこれらを1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを作る
	ComPtr <ID3D12Resource> vertexResource = nullptr;
	HRESULT hr;
	hr = device->CreateCommittedResource(
		&uploadHeapProperties, // ヒープの設定
		D3D12_HEAP_FLAG_NONE, // ヒープのフラグ
		&vertexResourceDesc, // リソースの設定
		D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの状態
		nullptr, // 初期化するリソース
		IID_PPV_ARGS(&vertexResource) // リソースのポインタ
	);
	assert(SUCCEEDED(hr));

	return vertexResource; // 変更点: ComPtr そのものを返す
}

// 変更点: 戻り値を ComPtr<ID3D12Resource> に変更
ComPtr<ID3D12Resource> CreateDepthStencilTexureResource(ID3D12Device* device, int32_t width, int32_t height) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = 1; // マルチサンプルはしない
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度ステンシル用のフラグを設定

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // デフォルトヒープを使用

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深度のクリア値
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度ステンシルのフォーマット

	ComPtr <ID3D12Resource> resource = nullptr; // 変更点: ComPtr を使用
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // ヒープの設定
		D3D12_HEAP_FLAG_NONE, // ヒープのフラグ
		&resourceDesc, // リソースの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // リソースの状態
		&depthClearValue, // 初期化するリソース
		IID_PPV_ARGS(&resource) // リソースのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

// 変更点: 戻り値を ComPtr<ID3D12DescriptorHeap> に変更
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	// ディスクリプタヒープの作成
	ComPtr <ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(
		&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	// ディスクリプタヒープの生成に失敗した場合はエラー
	assert(SUCCEEDED(hr));
	return descriptorHeap;
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

// 変更点: 戻り値を ComPtr<ID3D12Resource> に変更
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
	ComPtr <ID3D12Resource> resource = nullptr; // 変更点: ComPtr を使用
	HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

// 変更点: textureとdeviceの引数を ComPtr<ID3D12Resource> と ComPtr<ID3D12Device> に変更
[[nodiscard]]
ComPtr<ID3D12Resource> UploadTextureData(ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, ComPtr<ID3D12Device> device, ID3D12GraphicsCommandList* commandList) {
	// Mate情報を取得
	std::vector<D3D12_SUBRESOURCE_DATA>subresorce;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresorce); // 変更点: device.Get() を使用
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresorce.size()));
	ComPtr <ID3D12Resource> intermediateResource = CreatBufferResouce(device.Get(), intermediateSize); // 変更点: CreatBufferResouce に device.Get() を渡す
	UpdateSubresources(commandList, texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresorce.size()), subresorce.data()); // 変更点: texture.Get() と intermediateResource.Get() を使用

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
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

	// --- ウィンドウの初期化 ---
	WNDCLASS wc{};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//　カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスを登録する
	RegisterClass(&wc);
	//ウィンドウサイズを表す構造体に、クライアント領域
	RECT wrc = { 0,0,KclientWidth,KclientHeight };

	//ウィンドウサイズの生成
	HWND hand = CreateWindow(
		wc.lpszClassName, // ウィンドウクラス名
		L"CG2", // ウィンドウ名
		WS_OVERLAPPEDWINDOW, // ウィンドウスタイル
		CW_USEDEFAULT, // x座標
		CW_USEDEFAULT, // y座標
		wrc.right - wrc.left, // 幅
		wrc.bottom - wrc.top, // 高さ
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		wc.hInstance, // インスタンスハンドル
		nullptr); // ユーザーデータ

#ifdef _DEBUG
	ComPtr <ID3D12Debug1> debugController = nullptr; // 変更点: ComPtr を使用
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//　デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		/*debugController->SetEnableGPUBasedValidation(TRUE);*/
	}
	else {
		Log("Debug Layer is not supported.\n");
	}

#endif // DEBUG

	//ウィンドウを表示する
	ShowWindow(hand, SW_SHOW);

	// --- DirectX 12デバイスの初期化 ---
	// DXGIファクトリーの生成
	ComPtr <IDXGIFactory7> dxgiFactory = nullptr; // 変更点: ComPtr を使用
	//関数が成功したかどうかをSUCCEEDEDマクロで確認
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	// 使用するアダプター用の変数。最初にnullptrを入れておく
	ComPtr <IDXGIAdapter4> useAdpter = nullptr; // 変更点: ComPtr を使用
	// いい順アダプタを積む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdpter)) != DXGI_ERROR_NOT_FOUND; i++) {
		//アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdpter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		// ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		useAdpter = nullptr;// ソフトウェアアダプタは捨てる
	}
	// アダプタが見つからなかった場合はエラー
	assert(useAdpter != nullptr);

	ComPtr <ID3D12Device> device = nullptr; // 変更点: ComPtr を使用
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(
			useAdpter.Get(), // アダプタ
			featureLevels[i], // 機能レベル
			IID_PPV_ARGS(&device) // デバイスのポインタ
		);
		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Log(std::format("Feature Level:{}\n", featureLevelStrings[i]));
			break;
		}
	}
#ifdef _DEBUG
	ComPtr <ID3D12InfoQueue> infoQueue = nullptr; // 変更点: ComPtr を使用
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// ヤバイエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		//// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		//
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//　抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//　指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

	}

#endif // _DEBUG

	// デバイスが生成できなかった場合はエラー
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");//初期化完了のログを出す

	//コマンドキューを生成する
	ComPtr <ID3D12CommandQueue> commandQueue = nullptr; // 変更点: ComPtr を使用
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(
		&commandQueueDesc, // コマンドキューの設定
		IID_PPV_ARGS(&commandQueue) // コマンドキューのポインタ
	);
	// コマンドキューの生成に失敗した場合はエラー
	assert(SUCCEEDED(hr));

	//スワップチェーンを生成する
	ComPtr <IDXGISwapChain4> swapChain = nullptr; // 変更点: ComPtr を使用
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = KclientWidth; // スワップチェーンの幅
	swapChainDesc.Height = KclientHeight; // スワップチェーンの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // スワップチェーンのフォーマット
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングのサンプル数
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // スワップチェーンの使用方法
	swapChainDesc.BufferCount = 2; // バッファの数
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // スワップチェーンの効果
	//コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(), // コマンドキュー (変更点: Get() を使用)
		hand, // ウィンドウハンドル
		&swapChainDesc, // スワップチェーンの設定
		nullptr, // モニターのハンドル
		nullptr, // スワップチェーンのフラグ
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf())// スワップチェーンのポインタ (変更点: IID_PPV_ARGS を使用)
	);
	Log(std::format("CreateSwapChainForHwnd result HRESULT: 0x{:X}\n", hr)); // 追加
	assert(SUCCEEDED(hr));// スワップチェーンの生成に失敗した場合はエラー
	// コマンドアローケータを生成する
	ComPtr <ID3D12CommandAllocator> commandAllocator = nullptr; // 変更点: ComPtr を使用
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		IID_PPV_ARGS(&commandAllocator) // コマンドアローケータのポインタ
	);
	//コマンドリストを生成する
	ComPtr <ID3D12GraphicsCommandList> commandList = nullptr; // 変更点: ComPtr を使用
	hr = device->CreateCommandList(
		0, // コマンドリストのインデックス
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		commandAllocator.Get(), // コマンドアローケータ (変更点: Get() を使用)
		nullptr, // パイプラインステートオブジェクト
		IID_PPV_ARGS(&commandList) // コマンドリストのポインタ
	);
	assert(SUCCEEDED(hr));
	// --- ディスクリプタヒープの作成 ---
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	// ImGuiが一つ使うので、余裕を持たせて128個確保
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// SwapChainからResourceを取得する
	Microsoft::WRL::ComPtr <ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	// うまく取得できなかった場合はエラー
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // スワップチェーンのフォーマット
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // スワップチェーンの種類
	// ディスクリプタの先頭を取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandlee = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//RTVを2つ作るのでディスクリプタを２つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	//まず１つ目を作る
	rtvHandles[0] = rtvStartHandlee;
	device->CreateRenderTargetView(
		swapChainResources[0].Get(), // スワップチェーンのリソース
		&rtvDesc, // レンダーターゲットビューの設定
		rtvHandles[0] // レンダーターゲットビューのハンドル
	);
	//2つ目のディスクリプタを得る
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 2つ目を作る
	device->CreateRenderTargetView(
		swapChainResources[1].Get(), // スワップチェーンのリソース
		&rtvDesc, // レンダーターゲットビューの設定
		rtvHandles[1] // レンダーターゲットビューのハンドル
	);

	// --- Fenceの初期化 ---
	// 初期化値0でFenceを作る
	Microsoft::WRL::ComPtr <ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(
		fenceValue, // フェンスの初期値
		D3D12_FENCE_FLAG_NONE, // フェンスのフラグ
		IID_PPV_ARGS(&fence) // フェンスのポインタ
	);
	assert(SUCCEEDED(hr));
	// FenceのSignalを持つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent != nullptr);

	// --- dxcCompilerの初期化 ---
	Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// 現時点でincludeはしないが、includeに対応するための設定
	Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	// --- RootSignatureの作成 ---
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // シェーダーレジスタ番号
	descriptorRange[0].NumDescriptors = 1; // ディスクリプタの数
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameterの設定
	D3D12_ROOT_PARAMETER rootParameters[4]{};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う (materialResource)
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixelシェーダーの可視性
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う (WVP, TransformationMatrix)
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderシェーダー
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // ディスクリプタテーブルを使う (Texture SRV)
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixelシェーダーの可視性
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange; // ディスクリプタの配列
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // ディスクリプタの数
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う (DirectionalLight)
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixelシェーダーの可視性
	rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う


	descriptionRootSignature.pParameters = rootParameters; // RootParameterの配列
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // フィルタの種類
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // U座標のアドレスモード
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // V座標のアドレスモード
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // W座標のアドレスモード
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較関数
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // 最大LOD
	staticSamplers[0].ShaderRegister = 0; // シェーダーレジスタ番号
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // シェーダーの可視性
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // 静的サンプラーの数
	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	hr = device->CreateRootSignature(
		0, // シグネチャのインデックス
		signatureBlob->GetBufferPointer(), // シグネチャのバイナリ
		signatureBlob->GetBufferSize(), // シグネチャのサイズ
		IID_PPV_ARGS(&rootSignature) // シグネチャのポインタ
	);
	assert(SUCCEEDED(hr));

	// --- InputLayout、BlendState、RasterizerStateの設定 ---
	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = { };
	inputElementDescs[0].SemanticName = "POSITION"; // セマンティクス名
	inputElementDescs[0].SemanticIndex = 0; // セマンティクスのインデックス
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // フォーマット
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT; // フォーマット
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // フォーマット
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs; // 入力要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 入力要素の数

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// 全ての要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasiterzerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// --- Shaderのコンパイル ---
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = CompileShader(
		L"Object3D.VS.hlsl", // シェーダーのファイルパス
		L"vs_6_0", // プロファイル
		dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), logstream);
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr < IDxcBlob> pixelShaderBlob = CompileShader(
		L"Object3D.PS.hlsl", // シェーダーのファイルパス
		L"ps_6_0", // プロファイル
		dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), logstream);
	assert(pixelShaderBlob != nullptr);

	// --- GraphicsPipelineStateの設定と生成 ---
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get(); // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc; // 入力レイアウト
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() }; // 頂点シェーダー
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() }; // ピクセルシェーダー
	graphicsPipelineStateDesc.BlendState = blendDesc; // ブレンドステート
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // ラスタライザーステート
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1; // 書き込むRTVの数
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 書き込むRTVのフォーマット
	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むのかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	depthStencilDesc.DepthEnable = true; // 深度テストを有効にする
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 全ての深度値を書き込む
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 深度比較関数


	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc; // パイプラインステートに深度ステンシルの設定を適用
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度ステンシルのフォーマットを設定

	// 実際に生成
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc, // パイプラインステートの設定
		IID_PPV_ARGS(&graphicsPipelineState) // パイプラインステートのポインタ
	);
	assert(SUCCEEDED(hr));

	// --- リソースの生成とデータ設定 ---
	// Sprite用の頂点リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = CreatBufferResouce(device.Get(), sizeof(VertexData) * 6);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	// Sphere用のWVPリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = CreatBufferResouce(device.Get(), sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpData = nullptr;
	// 書き込むためのアドレス取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	// 単位行列を書き込んでおく
	wvpData->World = calculation.MakeIdentity4x4();
	wvpData->WVP = calculation.MakeIdentity4x4();

	constexpr uint32_t kSubdivision = 16; // 分割数

	constexpr uint32_t kVertexCount = kSubdivision * kSubdivision * 6;

	constexpr uint32_t kIndexCount = kVertexCount;

	//モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");

	// Sphere用の頂点リソース 
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = CreatBufferResouce(device.Get(), sizeof(VertexData) * modelData.vertices.size());

	// Sphere用の頂点バッファビューを作成する 
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// Sphere用のインデックスリソースとビュー
	Microsoft::WRL::ComPtr < ID3D12Resource> indexBuffer = CreatBufferResouce(device.Get(), sizeof(uint32_t) * kIndexCount);
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32ビット符号なし整数

	// Sphere用の頂点リソースにデータを書き込む 
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	// Sphere用のインデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// Sphere用のマテリアルリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = CreatBufferResouce(device.Get(), sizeof(Material));
	Material* materialData = nullptr;
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = true;


	// Sprite用のマテリアルリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreatBufferResouce(device.Get(), sizeof(Material));
	Material* materialDataSprite = nullptr;
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataSprite->enableLighting = false;

	// Sprite用の頂点データ
	VertexData* VertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&VertexDataSprite));

	//1枚目の三角形
	VertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	VertexDataSprite[0].texcoord = { 0.0f,1.0f };
	VertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	VertexDataSprite[1].texcoord = { 0.0f,0.0f };

	//2枚目の三角形
	VertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f }; 
	VertexDataSprite[2].texcoord = { 1.0f,1.0f };
	VertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	VertexDataSprite[3].texcoord = { 1.0f,0.0f };


	//Sprite用のTransformationMatrix用のリソースを作成
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = CreatBufferResouce(device.Get(), sizeof(Material));
	//データを書き込む
	TransformationMatrix* transformetionMatrixDataSprite = nullptr;
	//書き込むためのアドレスを取得
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformetionMatrixDataSprite));
	//単位行列を書き込んでおく
	transformetionMatrixDataSprite->World = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	transformetionMatrixDataSprite->WVP = calculation.MakeIdentity4x4();

	Microsoft::WRL::ComPtr < ID3D12Resource> indexResourceSprite = CreatBufferResouce(device.Get(), sizeof(uint32_t) * 6);
	D3D12_INDEX_BUFFER_VIEW indexBuffViewSprite{};
	// リソースの先頭のアドレスから使う
	indexBuffViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBuffViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//　インデックスはuint32_tとする
	indexBuffViewSprite.Format = DXGI_FORMAT_R32_UINT;

	// DirectionalLight用のリソース
	Microsoft::WRL::ComPtr < ID3D12Resource> directionalLightResource = CreatBufferResouce(device.Get(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f,1.0f, 1.0f, 1.0f }; // 光の色
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f }; // 光の方向
	directionalLightData->intensity = 1.0f; // 光の強度
	directionalLightData->direction = calculation.Normalize(directionalLightData->direction); // 光の方向を正規化

	materialData->uvTransform = calculation.MakeIdentity4x4(); // UV変換行列を単位行列に初期化
	materialDataSprite->uvTransform = calculation.MakeIdentity4x4(); // SpriteのUV変換行列も単位行列に初期化
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f}
	};
	// インデックスデータを作成
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;

	// --- ビューポートとシザー矩形 ---
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = (float)KclientWidth;
	viewport.Height = (float)KclientHeight;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// シザー矩形
	D3D12_RECT scissorRect{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	scissorRect.left = 0;
	scissorRect.right = KclientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = KclientHeight;

	// --- ImGuiの初期化 ---
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hand);
	// ImGuiはsrvDescriptorHeapの先頭を使うため、そのCPU/GPUハンドルを渡す
	ImGui_ImplDX12_Init(device.Get(), swapChainDesc.BufferCount, rtvDesc.Format, srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// --- テクスチャのロードとアップロード、SRVの作成 ---
	// テクスチャ1のロードとアップロード
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata metadata = mipImages.GetMetadata();
	// 変更点: CreateTextureResource の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = CreateTextureResource(device.Get(), metadata);
	// 変更点: UploadTextureData の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr < ID3D12Resource> textureUploadHeap = UploadTextureData(textureResource.Get(), mipImages, device.Get(), commandList.Get());

	// テクスチャ2のロードとアップロード
	DirectX::ScratchImage mipImage2 = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata metadata2 = mipImage2.GetMetadata();
	// 変更点: CreateTextureResource の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = CreateTextureResource(device.Get(), metadata2); // metadata2を使用
	// 変更点: UploadTextureData の戻り値を ComPtr に変更
	Microsoft::WRL::ComPtr <ID3D12Resource> textureUploadHeap2 = UploadTextureData(textureResource2.Get(), mipImage2, device.Get(), commandList.Get());

	// コマンドリストを閉じて実行、アップロード完了を待機
	hr = commandList->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* commandListsForUpload[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandListsForUpload);
	uint64_t currentFenceValue = ++fenceValue;
	commandQueue->Signal(fence.Get(), currentFenceValue);
	if (fence->GetCompletedValue() < currentFenceValue) {
		fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// コマンドリストとアロケーターをリセットして、メインループで再度使用できるようにする
	hr = commandAllocator->Reset(); assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr); assert(SUCCEEDED(hr));

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
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandelCPU = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), desriptorSizeSRV, 1); // Index 1
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandelGPU = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), desriptorSizeSRV, 1); // Index 1

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandelCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), desriptorSizeSRV, 2); // Index 2
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandelGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), desriptorSizeSRV, 2); // Index 2

	// SRVの生成
	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandelCPU);
	device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandelCPU2); // srvDesc2を使用

	// --- 深度ステンシルリソースとDSVヒープの作成 ---
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStencilTexureResource(device.Get(), KclientWidth, KclientHeight);
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度ステンシルのフォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 深度ステンシルの種類

	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Sprite用のTransform
	Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	bool useMonsterBall = true;
	MSG msg{};
	// --- メインループ ---
	//ウィンドウの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		//メッセージがあるか確認
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			//メッセージがあったら処理する
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			// Sphereの回転
			transform.rotate.y += 0.01f;
			Calculation::Matrix4x4 woldMatrix = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

			Calculation::Matrix4x4 cameraMatrix = calculation.MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Calculation::Matrix4x4 viewMatrix = calculation.Inverse(cameraMatrix);
			Calculation::Matrix4x4 projectionMatrix = calculation.MakePerspectiveFovMatrix(0.45f, float(KclientWidth) / float(KclientHeight), 0.1f, 100.0f);
			Calculation::Matrix4x4 worldViewProjectionMatrix = calculation.Multiply(woldMatrix, calculation.Multiply(viewMatrix, projectionMatrix));
			wvpData->WVP = worldViewProjectionMatrix; // SphereのWVPを更新
			wvpData->World = woldMatrix; // SphereのWorldを更新

			// Spriteの変換行列
			Calculation::Matrix4x4 worldMatrixSprite = calculation.MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Calculation::Matrix4x4 viewMatrixSprite = calculation.MakeIdentity4x4(); // Spriteは通常View変換しない
			Calculation::Matrix4x4 projectionMatrixSprite = calculation.MakeOrthographicMatrix(0.0f, 0.0f, float(KclientWidth), float(KclientHeight), 0.0f, 100.0f); // クライアントサイズをそのまま使う
			Calculation::Matrix4x4 worldViewProjectionMatrixSprite = calculation.Multiply(worldMatrixSprite, calculation.Multiply(viewMatrixSprite, projectionMatrixSprite));
			transformetionMatrixDataSprite->WVP = worldViewProjectionMatrixSprite; // SpriteのWVPを更新
			transformetionMatrixDataSprite->World = worldMatrixSprite; // SpriteのWorldを更新

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

			ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
			ImGui::End();

			Calculation::Matrix4x4 uvTransformMatrix = calculation.MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformMatrix = calculation.Multiply(uvTransformMatrix, calculation.MakeRotationZMatrix(uvTransformSprite.rotate.z));
			uvTransformMatrix = calculation.Multiply(uvTransformMatrix, calculation.MakeTranslationMatrix(uvTransformSprite.translate));
			materialDataSprite->uvTransform = uvTransformMatrix; // UV変換行列を更新

			ImGui::Render();
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			// TransitionBarrrierの設定
			D3D12_RESOURCE_BARRIER barrier{};
			// 今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			// Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			// バリアのリソース
			barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
			// 遷移前(現在)のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			// 遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			// TransitionBarrierを張る
			commandList->ResourceBarrier(1, &barrier);

			// 深度ステンシルビューのハンドルを取得
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			// 描画先のRTVとDSVを設定
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], FALSE, &dsvHandle);

			// 指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			// 深度バッファをクリア
			commandList->ClearDepthStencilView(
				dsvHandle, // 深度ステンシルビューのハンドル
				D3D12_CLEAR_FLAG_DEPTH, // クリアするフラグ
				1.0f, // 深度値のクリア値
				0, // ステンシル値のクリア値
				0, nullptr // クリアのオプション
			);

			// 描画用のDescriptorHeapを設定 (SRV/CBV/UAV用)
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);

			// ViewportとScissorを設定
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);

			// RootSignatureとPipelineStateを設定 (Sphere用)
			commandList->SetGraphicsRootSignature(rootSignature.Get());
			commandList->SetPipelineState(graphicsPipelineState.Get());
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // Sphereの頂点バッファ
			commandList->IASetIndexBuffer(&indexBufferView); // Sphereのインデックスバッファ
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 形状を設定

			// Sphere用のリソースをバインド
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress()); // Sphereのマテリアル
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress()); // SphereのWVP
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress()); // DirectionalLight
			commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandelGPU2 : textureSrvHandelGPU); // Sphereのテクスチャ

			// Sphereを描画
			commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

			// Sprite用のリソースをバインド (必要に応じてPipelineStateを再設定することも検討)
			// マテリアルCBufferの場所を設定 (Sprite用)
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress()); // Spriteのマテリアル
			commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress()); // SpriteのWVP
			// Spriteのテクスチャ (Sphereと同じSRVスロットを使う場合)
			commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandelGPU); // 例えばuvCheckerをSpriteに使う

			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite); // Spriteの頂点バッファ

			commandList->IASetIndexBuffer(&indexBuffViewSprite); // Spriteのインデックスバッファ
			// Spriteを描画
			commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// ImGuiの描画
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

			// RenderTargetからPresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			commandList->ResourceBarrier(1, &barrier);

			// コマンドリストの内容を確定させる
			hr = commandList->Close(); assert(SUCCEEDED(hr));
			// GPUにコマンドリストを実行させる
			ID3D12CommandList* commandLists[] = { commandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);
			// GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);

			// Feneeceの値を更新し、GPUの処理完了を待つ
			fenceValue++;
			commandQueue->Signal(fence.Get(), fenceValue);
			if (fence->GetCompletedValue() < fenceValue) {
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				WaitForSingleObject(fenceEvent, INFINITE);
			}
			// 次のフレーム用のコマンドリストを準備
			hr = commandAllocator->Reset(); assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator.Get(), nullptr); assert(SUCCEEDED(hr));
		}
	}
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているかどうか、どうにもできない場合が多いのでassertにしておく
	Log("Hello, DirectX!\n");
	Log(ConvertString(std::format(L"windth\n", KclientWidth)));

	// --- 解放処理 ---
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CloseHandle(fenceEvent);

	CloseWindow(hand);
	CoUninitialize();

	return 0;
}

//GE3