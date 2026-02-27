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
#include"TextureManager.h"
#include"Object3dCommon.h"
#include"Object3d.h"
#include"Model.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

// ComPtr を簡単に使うために名前空間をusing
using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;

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
	Object3dCommon* object3dCommon = nullptr;

	// Inputの初期化
	input = new Input();
	// WinAppの初期化
	winApp = new WinApp();
	// DirectXCommonの初期化
	dxCommon = new DirectXCommon();
	// SpriteCommonの初期化
	spriteCommon = new SpriteCommon();
	// Object3dCommonの初期化
	object3dCommon = new Object3dCommon();

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
	
	// 1. 各種コモン・マネージャーの初期化
	winApp->Initialize();
	input->Initialize(winApp);
	dxCommon->Initialize(winApp);
	dxCommon->ImGuiInitialize();
	TextureManager::GetInstance()->Initialize();

	spriteCommon->Initialize(dxCommon);
	object3dCommon->Initialize(dxCommon); // 3D共通設定

	// 2. テクスチャのロード
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png", dxCommon);
	TextureManager::GetInstance()->LoadTexture("resources/monsterBall.png", dxCommon);

	// 3. モデルの生成と初期化（ファイル読み込み〜GPUバッファ作成まで完了）
	Model* model = new Model();
	model->Initialize(dxCommon, "resources", "plane.obj");

	// 4. 3Dオブジェクトの生成と設定（座標とモデルの紐付け）
	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dCommon);
	object3d->SetModel(model); // このオブジェクトは plane.obj を使う

	// 5. スプライトの初期化
	Sprite* uvChecker = new Sprite();
	uvChecker->Initialize(spriteCommon, "resources/uvChecker.png");
	Sprite* monsterBall = new Sprite();
	monsterBall->Initialize(spriteCommon, "resources/monsterBall.png");
	HRESULT hr = S_OK; // これを追加

	uvChecker->SetPosition({ 0.0f, 0.0f });
	/*uvChecker->SetSize({ 640.0f, 320.0f });*/
	monsterBall->SetPosition({ 640.0f, 360.0f });
	monsterBall->SetSize({ 128.0f, 128.0f });

	// --- メインループ ---
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {
		//メッセージがあるか確認
		if (winApp->ProcessMassage()) {
			break;
		}
		input->Update(); // キー入力の更新
		// --- 更新処理 ---
		dxCommon->ImGuiPreDraw();

		uvChecker->Update();
		monsterBall->Update();

		// 3Dオブジェクトの更新（内部で行列計算と転送が行われる）
		object3d->SetCameraTransform({ {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 2.0f, -10.0f} }); // カメラの位置をセット
		object3d->SetTranslate({ 0.0f, 0.0f, 0.0f }); // オブジェクトの位置をセット)
		object3d->SetScale({ 1.0f, 1.0f, 1.0f }); // オブジェクトのスケールをセット)
		object3d->Update();

		// 開発用UIの処理
		ImGui::ShowDemoWindow();
		// 全体をまとめる親ウィンドウ
		ImGui::Begin("DebugManager"); 
		if (ImGui::BeginTabBar("MainTabBar")) {

			// uvChecker 用のタブ
			if (ImGui::BeginTabItem("UV Checker")) {
				uvChecker->ImGui();
				ImGui::EndTabItem();
			}

			// monsterBall 用のタブ
			if (ImGui::BeginTabItem("Monster Ball")) {
				monsterBall->ImGui();
				ImGui::EndTabItem();
			}

			// 他にデバッグしたいものがあればここに追加
			// if (ImGui::BeginTabItem("Camera")) { ... }

			ImGui::EndTabBar();
		}
		ImGui::End();
		// --- 描画処理 ---
		// 1. 3D描画
		object3dCommon->PreDraw(); // 3D用の共通設定（PSO, RootSignatureなど）をセット
		object3d->Draw();            // 自身の行列をセットして、Modelに描画を依頼

		// 2. スプライト描画
		spriteCommon->PreDraw();   // スプライト用の共通設定をセット
		uvChecker->Draw();
		monsterBall->Draw();

		// 3. 画面表示
		dxCommon->ImGuiPostDraw();
		dxCommon->PostDraw();
	}
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているかどうか、どうにもできない場合が多いのでassertにしておく
	Log("Hello, DirectX!\n");
	Log(ConvertString(std::format(L"windth\n", WinApp::KclientWidth)));

	CloseHandle(dxCommon->GetFenceEvent());
	dxCommon->ImGuiFinalize();
	winApp->Finalize();
	TextureManager::GetInstance()->Finalize();

	delete input;
	delete winApp;
	delete dxCommon;
	delete spriteCommon;
	delete uvChecker;
	delete monsterBall;
	delete object3dCommon;
	delete object3d;
	delete model;

	return 0;
}

// マージ確認用コメント