#include "MyGame.h"
#include "GameScene.h"

void MyGame::Initialize() {
	// 基底クラスの初期化
	Framework::Initialize();

	// 最初のアクティブシーンをゲームシーンに設定
	sceneManager->ChangeScene(new GameScene());
}

void MyGame::Update() {
	// 基底クラスの更新
	Framework::Update();
}

void MyGame::Draw() {
	// 描画前処理
	dxCommon->PreDraw();
	srvManager->PreDraw();

	// シーンマネージャ経由で現在のシーンを描画
	sceneManager->Draw();

	// ImGuiの描画
	dxCommon->ImGuiPostDraw();

	// 画面表示
	dxCommon->PostDraw();
}

void MyGame::Finalize() {
	// 基底クラスの終了処理
	Framework::Finalize();
}
