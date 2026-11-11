#pragma once
#include "Framework.h"
#include "SceneFactory.h"
#include "SceneTransition.h"
#include "ScreenEffectController.h"
#include <functional>
#include <memory>
#include <string>

/// <summary>
/// このゲームの起動・終了と、最初のシーン・ゲーム全体の設定を決める（エンジンの Framework を継承）。
/// 画面の効果の手動 ON/OFF（ScreenEffectController）と、シーン遷移の演出（SceneTransition）を持つ。
/// </summary>
class MyGame : public Framework {
public:
	// Framework::GetInstance() を MyGame として返す（シーンから ScreenEffectController などへ届くように）
	static MyGame* GetInstance() { return static_cast<MyGame*>(Framework::GetInstance()); }

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

	ScreenEffectController* GetScreenEffect() const { return screenEffect.get(); }

	// 演出つきのシーン遷移（暗転 → 差し替え → 明転）。遷移中の再要求は無視する
	void RequestTransition(const std::string& sceneName, const TransitionParams& params = SceneTransition::kBattleStartParams);
	// シーンを変えない暗転。真っ暗になったフレームで onBlack を 1 回呼び、その後明転する。遷移中は無視する
	void RequestFade(const TransitionParams& params, std::function<void()> onBlack);

private:
	std::unique_ptr<SceneFactory> sceneFactory = nullptr;
	std::unique_ptr<ScreenEffectController> screenEffect = nullptr;
	std::unique_ptr<SceneTransition> sceneTransition = nullptr;
};
