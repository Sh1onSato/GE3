#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "PauseTuningMenu.h"
#include "BitmapText.h"
#include "Structs.h"
#include "Object3d.h"
#include "Skybox.h"
#include "Model.h"
#include "Camera.h"
#include "ScreenEffectController.h"
#include "ArenaLayout.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

/// <summary>
/// タイトルシーン：START / SETTINGS / QUIT のメインメニューと、マウス感度・目線の高さの設定画面を持つ。
/// GameSettings（シングルトン）経由で値をGameSceneへ引き継ぐ。START選択でSceneManager経由のフェード遷移でゲーム開始。
/// </summary>
class TitleScene : public BaseScene {
public:
	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	// 画面状態（フェードはSceneManager/SceneTransitionが担当）
	enum class State { Main, Settings };

	// メインメニューの項目（kMenuLabelsと並びを一致させる）
	enum MenuIndex { kMenuStart = 0, kMenuSettings, kMenuQuit, kMenuCount };

	void InitializeSettingsMenu();
	void InitializeTexts();
	// 3D背景（カメラ・スカイボックス・床・壁・ボス・ポストプロセス）の生成と後始末
	void InitializeBackground3d();
	void InitializeArenaObjects();
	// BitmapTextを初期化し、倍率・文字列・色を設定する（位置は呼び出し側で決める）
	void SetupText(BitmapText& text, size_t maxChars, const std::string& str, float scale, const Vector4& color);
	// 画面中央揃えで指定Yに配置する
	void SetCenteredPosition(BitmapText& text, float y);

	void UpdateMain();
	void UpdateSettings();
	void UpdateMenuColors();
	void UpdateTexts();
	// カメラ周回・ボス浮遊・3Dオブジェクト行列の更新
	void UpdateBackground3d();
	// 暗幕の濃さをメイン/設定画面で切り替える
	void UpdateDimOverlay();

	void DrawBackground3d();
	void DrawShadowPass();
	void DrawMain();
	void DrawSettings();

	State state = State::Main;
	float blinkTimer = 0.0f;
	int selectedIndex = kMenuStart;

	// --- 3D背景（GameSceneと同じアリーナを、ボスを見ながらカメラが周回して映す） ---
	Camera* camera = nullptr; // CameraManagerが所有。Finalizeで登録解除する
	float backgroundTimer = 0.0f;
	std::unique_ptr<Model> cubeModel = nullptr;
	std::unique_ptr<Skybox> skybox = nullptr;
	std::unique_ptr<Object3d> floor = nullptr;
	std::vector<std::unique_ptr<Object3d>> walls;
	std::unique_ptr<Object3d> boss = nullptr;
	ScreenEffectController* screenEffect = nullptr; // MyGameが所有する画面の効果の窓口（非所有）

	std::unique_ptr<Sprite> background = nullptr;  // 3D背景の上に重ねる半透明の暗幕
	PauseTuningMenu settingsMenu;                      // マウス感度・目線の高さの調整項目

	// メイン画面のテキスト
	BitmapText titleText;
	std::array<BitmapText, kMenuCount> menuTexts;
	BitmapText cursorText; // 選択中の項目の左に出す ">"
	BitmapText guideText;

	// 設定画面のテキスト
	BitmapText settingsHeading;
	std::array<BitmapText, 2> settingsLabels; // 設定項目の行ラベル（PauseTuningMenuのItem順と一致）
	BitmapText settingsHint;

	// 仮タイトル。将来ロゴ画像に差し替える際はここと titleText の描画を置き換える
	static constexpr const char* kTitleText = "ARENA";
	static constexpr const char* kMenuLabels[kMenuCount] = { "START", "SETTINGS", "QUIT" };
	static constexpr const char* kSettingsLabelTexts[2] = { "MOUSE SENSITIVITY", "EYE HEIGHT" };

	static constexpr float kDeltaTime = 1.0f / 60.0f;
	static constexpr float kBlinkSpeed = 6.0f; // 選択項目の明滅の角速度(rad/秒)

	static constexpr float kTitleScale = 10.0f;
	static constexpr float kTitlePositionY = 180.0f;
	static constexpr float kMenuScale = 3.0f;
	static constexpr float kMenuPositionY = 400.0f;
	static constexpr float kMenuSpacing = 55.0f;
	static constexpr float kCursorGap = 12.0f; // ">" と項目文字の間の余白
	static constexpr float kGuideScale = 2.0f;
	static constexpr float kGuideBottomMargin = 60.0f;
	static constexpr float kHeadingScale = 4.0f;
	static constexpr float kHeadingPositionY = 30.0f;
	static constexpr float kSettingsHintPositionY = 600.0f;

	static constexpr Vector4 kSelectedColor = { 1.0f, 1.0f, 0.2f, 1.0f };
	static constexpr Vector4 kUnselectedColor = { 0.5f, 0.5f, 0.55f, 1.0f };
	static constexpr Vector4 kTitleColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr float kBlinkAlphaMin = 0.6f;
	static constexpr float kBlinkAlphaAmplitude = 0.4f;

	// 暗幕（背景の3Dの上に重ねる）。設定画面は文字を読みやすくするため濃くする
	static constexpr Vector3 kDimColor = { 0.05f, 0.05f, 0.08f };
	static constexpr float kDimAlphaMain = 0.35f;
	static constexpr float kDimAlphaSettings = 0.7f;

	// 背景カメラ。GameScene側の"MainCamera"と競合しないよう別名で登録し、Finalizeで消す
	static constexpr const char* kCameraName = "TitleCamera";
	// 周回中心はアリーナ中央。ボス(z=20)を中心に半径22で回ると外壁(±30)の外に出てしまうため、
	// 中心を原点にして壁の内側を回り、注視点だけをボスに向ける
	static constexpr Vector3 kOrbitCenter = { 0.0f, 0.0f, 0.0f };
	static constexpr float kOrbitRadius = 15.0f;       // 角プラットフォーム(約19.8)の内側、ボス(20)との最接近は約5
	static constexpr float kOrbitHeight = 7.0f;        // プラットフォーム上段(高さ5)より上から見下ろす
	static constexpr float kOrbitAngularSpeed = 0.12f; // rad/秒
	static constexpr float kLookAtHeightOffset = 0.0f; // 注視点=ボス中心(胴体)からの上下補正

	// ボスの浮遊（控えめな上下動のみ）
	static constexpr float kBossFloatAmplitude = 0.15f;
	static constexpr float kBossFloatSpeed = 1.2f; // rad/秒
};
