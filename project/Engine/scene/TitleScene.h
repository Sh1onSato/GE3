#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "SettingsMenu.h"
#include "Structs.h"
#include <memory>

/// <summary>
/// タイトルシーン：プレイ開始前にマウス感度・目線の高さを調整できるワンクッション画面。
/// GameSettings（シングルトン）経由で値をGameSceneへ引き継ぐ。Enterキーでゲーム開始。
/// </summary>
class TitleScene : public BaseScene {
public:
	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	std::unique_ptr<Sprite> background = nullptr; // 全画面の暗い背景
	SettingsMenu settingsMenu;                     // マウス感度・目線の高さの調整項目

	// 「準備完了・Enterで開始」を示す点滅バー（新規フォント文字を増やさないための文字なし表現）
	std::unique_ptr<Sprite> promptBar = nullptr;
	float blinkTimer = 0.0f;
	static constexpr float kBlinkSpeed = 4.0f; // sin波の角速度(rad/秒)
	static constexpr Vector2 kPromptBarPosition = { 540.0f, 620.0f };
	static constexpr Vector2 kPromptBarSize = { 200.0f, 20.0f };
};
