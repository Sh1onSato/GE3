#include "MyGame.h"
#include "GameTime.h"
#include "Screen.h"

namespace {
	// ウィンドウのタイトル（旧 GE3 と同じ）
	constexpr const wchar_t* kWindowTitle = L"CG2";
	// ディレクショナルライトの影のバイアス（旧 GE3 は 0.0025。エンジンの既定 0.0005 だと見た目が変わるため合わせる）
	constexpr float kShadowBias = 0.0025f;
}

void MyGame::Initialize() {
	// 基底クラスの初期化
	Framework::Initialize();

	winApp->SetTitle(kWindowTitle);

	// 画面の縦横比がベース解像度と違うときは、ゲーム座標をベースのままにして黒帯を出す
	// （UI の座標は 1280x720 を前提に置いているため）
	Screen::SetScaleMode(Screen::ScaleMode::kLetterbox);

	// 影のバイアス
	object3dCommon->GetShadowBias() = kShadowBias;

	// シーンファクトリーの生成
	sceneFactory = std::make_unique<SceneFactory>();
	// シーンマネージャにファクトリーをセット
	sceneManager->SetSceneFactory(sceneFactory.get());

	// 全部のシーンで使う画面の効果の既定（旧 GE3 はブルームが常に ON。ScreenEffectController が毎フレーム同じ値を書く）
	PostEffectSettings defaultEffect;
	defaultEffect.bloomEnabled = true;
	defaultEffect.bloomThreshold = ScreenEffectController::kBloomThreshold;
	defaultEffect.bloomIntensity = 1.0f;
	defaultEffect.bloomRadius = ScreenEffectController::kBloomRadius;
	GetPostProcess()->SetDefaultSettings(defaultEffect);

	// 画面の効果の手動 ON/OFF と一瞬の演出（シーンをまたいで保つ）
	screenEffect = std::make_unique<ScreenEffectController>();
	screenEffect->Initialize(GetPostProcess());

	// シーン遷移の演出（SceneManager は所有しない）
	sceneTransition = std::make_unique<SceneTransition>();
	sceneTransition->Initialize(screenEffect.get());
	sceneManager->SetTransition(sceneTransition.get());

	// 最初のアクティブシーン（真っ暗から明転）
	sceneTransition->SetParams(SceneTransition::kBattleStartParams);
	sceneManager->StartFromBlack("TITLE");
}

void MyGame::Update() {
	// 基底クラスの更新（SceneManager::Update の中でシーンの更新・遷移演出が進み、シーンの切り替え時に PostProcess の設定が既定へ戻る）
	Framework::Update();

	// 一瞬の演出の時間を進めて、PostProcess の設定へ書き込む（シーンの更新の後。ポーズ中も進めるので unscaled）
	screenEffect->Update(Time::GetUnscaledDeltaTime());
	screenEffect->Apply();
}

void MyGame::Draw() {
	// 描画前処理
	dxCommon->PreDraw();
	srvManager->PreDraw();
	// 内部解像度を下げている・画面の効果を掛けるときは、3D の描画先を別の物にする（SceneManager がシーンの Draw の後で画面へ写す）
	renderScaler->BeginScene();

	// シーンマネージャ経由で現在のシーンを描画
	sceneManager->Draw();

	// ImGuiの描画（Debugビルドのみ）
#ifdef _DEBUG
	ImGuiManager::GetInstance()->End();
	ImGuiManager::GetInstance()->Draw();
#endif

	// 画面表示
	dxCommon->PostDraw();
}

void MyGame::RequestTransition(const std::string& sceneName, const TransitionParams& params) {
	// 遷移中に設定を書き換えると演出が壊れるので、遷移中の要求は無視する
	if (sceneManager->IsTransitioning()) {
		return;
	}
	sceneTransition->SetParams(params);
	sceneManager->ChangeSceneWithTransition(sceneName);
}

void MyGame::RequestFade(const TransitionParams& params, std::function<void()> onBlack) {
	if (sceneManager->IsTransitioning()) {
		return;
	}
	sceneTransition->SetParams(params);
	sceneManager->RequestFade(std::move(onBlack));
}

void MyGame::Finalize() {
	// 遷移演出・効果は Framework の終了処理（TextureManager などの解放）より前に消す
	// （Sprite が画像の使用数を戻すため）
	sceneManager->SetTransition(nullptr);
	sceneTransition.reset();
	screenEffect.reset();

	// 基底クラスの終了処理
	Framework::Finalize();
}
