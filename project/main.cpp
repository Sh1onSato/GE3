#include<Windows.h>
#include<cstdint>
#include<string>
#include<filesystem>
#include<fstream>
#include<chrono>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
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
#include"Camera.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakCheker leakChecker; // DirectXCommon.h に移動したもの
	CoInitializeEx(0, COINIT_MULTITHREADED);
	SetUnhandledExceptionFilter(Logger::ExportDump); // Logger に移動したもの

	Calculation calculation;
	Input* input = nullptr;
	WinApp* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;
	SpriteCommon* spriteCommon = nullptr;
	Object3dCommon* object3dCommon = nullptr;

	// インスタンス生成
	input = new Input();
	winApp = new WinApp();
	dxCommon = new DirectXCommon();
	spriteCommon = new SpriteCommon();
	object3dCommon = new Object3dCommon();
	Camera* camera = new Camera();
	SrvManager* srvManager = new SrvManager();

	// 1. 各種コモン・マネージャーの初期化
	winApp->Initialize();
	assert(winApp != nullptr);
	input->Initialize(winApp);
	dxCommon->Initialize(winApp);
	srvManager->Initialize(dxCommon);
	dxCommon->ImGuiInitialize(srvManager);

	TextureManager::GetInstance()->Initialize(dxCommon, srvManager);

	spriteCommon->Initialize(dxCommon, srvManager);
	object3dCommon->Initialize(dxCommon, srvManager); 

	// 2. テクスチャのロード
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");

	// 3. モデルの生成と初期化
	Model* model = new Model();
	model->Initialize(dxCommon, "Resources", "plane.obj");

	// 4. 3Dオブジェクトの生成と設定
	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dCommon);
	object3d->SetModel(model);
	object3d->SetCamera(camera); 

	Object3d* object3d2 = new Object3d();
	object3d2->Initialize(object3dCommon);
	object3d2->SetModel(model);
	object3d2->SetCamera(camera);
	object3d2->SetTranslate({ 2.0f, 0.0f, 0.0f });
	object3d2->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色に設定

	// 5. スプライトの初期化
	Sprite* uvChecker = new Sprite();
	uvChecker->Initialize(spriteCommon, "Resources/uvChecker.png");
	Sprite* monsterBall = new Sprite();
	monsterBall->Initialize(spriteCommon, "Resources/monsterBall.png");

	uvChecker->SetPosition({ 0.0f, 0.0f });
	monsterBall->SetPosition({ 640.0f, 360.0f });
	monsterBall->SetSize({ 128.0f, 128.0f });

	// --- メインループ ---
	while (true) {
		if (winApp->ProcessMassage()) {
			break;
		}
		input->Update(); 
		
		dxCommon->ImGuiPreDraw();

		uvChecker->Update();
		monsterBall->Update();

		camera->Update();
		// 3Dオブジェクトの更新（ImGuiで値をいじるため、ここでの固定値セットは削除）
		object3d->Update();
		object3d2->Update();

		// デバッグUI
		ImGui::ShowDemoWindow();
		ImGui::Begin("DebugManager"); 
		if (ImGui::BeginTabBar("MainTabBar")) {
			if (ImGui::BeginTabItem("UV Checker")) {
				uvChecker->ImGui();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Monster Ball")) {
				monsterBall->ImGui();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("3D Object 1")) {
				object3d->ImGui();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("3D Object 2")) {
				object3d2->ImGui();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Camera")) {
				camera->ImGui();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		// --- 描画処理 ---
		dxCommon->PreDraw(); 
		srvManager->PreDraw();
		
		// 1. 3D描画
		object3dCommon->PreDraw(); 
		object3d->Draw();
		object3d2->Draw();

		// 2. スプライト描画
		spriteCommon->PreDraw();   
		uvChecker->Draw();
		monsterBall->Draw();

		// 3. 画面表示
		dxCommon->ImGuiPostDraw();
		dxCommon->PostDraw();
	}

	// --- 解放処理（安全な順序） ---
	
	// 1. オブジェクト類
	delete camera;
	delete srvManager;
	delete object3d;
	delete object3d2;
	delete monsterBall;
	delete uvChecker;

	// 2. モデル類
	delete model;

	// 3. コモン類（DirectXCommonに依存）
	delete object3dCommon;
	delete spriteCommon;

	// 4. マネージャー・システム系
	TextureManager::GetInstance()->Finalize();
	delete input;

	// 5. DirectX基盤
	dxCommon->ImGuiFinalize();
	CloseHandle(dxCommon->GetFenceEvent());
	delete dxCommon;

	// 6. OS基盤
	winApp->Finalize();
	delete winApp;

	CoUninitialize();
	return 0;
}
