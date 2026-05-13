#include "Framework.h"
#include "ModelManager.h"
#include "CameraManager.h"

Framework* Framework::instance = nullptr;

void Framework::Run() {
	// 初期化
	Initialize();

	// メインループ
	while (true) {
		// ウィンドウメッセージ処理
		if (winApp->ProcessMassage()) {
			break;
		}

		// 更新
		Update();

		// 終了リクエストがあればループを抜ける
		if (IsEndRequest()) {
			break;
		}

		// 描画
		Draw();
	}

	// 終了
	Finalize();
}

void Framework::Initialize() {
	instance = this;
	winApp = new WinApp();
	winApp->Initialize();

	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	srvManager = new SrvManager();
	srvManager->Initialize(dxCommon);

	input = new Input();
	input->Initialize(winApp);

	dxCommon->ImGuiInitialize(srvManager);

	TextureManager::GetInstance()->Initialize(dxCommon, srvManager);
	ModelManager::GetInstance()->Initialize(dxCommon);

	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon, srvManager);

	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxCommon, srvManager);

	particleCommon = new ParticleCommon();
	particleCommon->Initialize(dxCommon, srvManager);

	CameraManager::GetInstance()->Initialize();

	sceneManager = new SceneManager();
}

void Framework::Update() {
	input->Update();
	dxCommon->ImGuiPreDraw();
	sceneManager->Update();
}

void Framework::Finalize() {
	sceneManager->Finalize();
	delete sceneManager;

	// 各種マネージャーの終了処理
	ModelManager::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();

	// 解放処理
	delete particleCommon;
	delete object3dCommon;
	delete spriteCommon;
	delete input;
	
	dxCommon->ImGuiFinalize();
	CloseHandle(dxCommon->GetFenceEvent());
	delete dxCommon;

	delete srvManager;

	winApp->Finalize();
	delete winApp;
}
