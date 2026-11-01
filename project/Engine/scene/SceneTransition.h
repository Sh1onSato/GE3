#pragma once
#include "Sprite.h"
#include "Structs.h"
#include <memory>
#include <vector>

// 遷移演出の見た目スタイル（追加時はSceneTransition::UpdateVisualのswitchに足す）
enum class TransitionStyle {
	Fade,   // 全画面フェード
	Tiles,  // タイルディゾルブ
};

// 遷移演出の設定（幕の色と各区間の時間[秒]）
struct TransitionParams {
	Vector4 color;          // 幕の色（rgbのみ使用。アルファは演出側で決める）
	float fadeOutDuration;  // 暗転にかける時間
	float holdDuration;     // 真っ暗で止める時間
	float fadeInDuration;   // 明転にかける時間
	TransitionStyle style = TransitionStyle::Fade; // 見た目スタイル（既存の集成体初期化を壊さないよう末尾）
	float glitchStrength = 0.0f; // 遷移中にかける画面グリッチの最大強度(0=なし、1=最大)
};

/// <summary>
/// シーン遷移演出（フェード）クラス。SceneManagerが所有する。
/// FadeOut（暗転）→ Hold（真っ暗で停止）→ シーン差し替え → FadeIn（明転）の流れを管理する。
/// 見た目はUpdateVisual()に集約しているため、スタイル追加はこのクラス内だけで済む。
/// </summary>
class SceneTransition {
public:
	enum class State { None, FadeOut, Hold, FadeIn };

	void Initialize();
	// 暗転開始（FadeOutから）
	void Start(const TransitionParams& p = kBattleStartParams);
	// 真っ暗な状態から明転開始（起動直後の最初のシーン用）
	void StartFromBlack(const TransitionParams& p = kBattleStartParams);
	// 暗転完了後、シーン差し替えを済ませてから呼ぶ。FadeInへ進める
	void BeginFadeIn();

	void Update();
	void Draw();

	// 遷移演出中か（FadeOut / Hold / FadeIn）
	bool IsActive() const { return state != State::None; }
	// 暗転中か（FadeOut / Hold）。この間はシーンの更新と入力を止める
	bool IsFadingOut() const { return state == State::FadeOut || state == State::Hold; }
	// 真っ暗になり、シーンを差し替えるべきフレームか
	bool IsReadyToSwitch() const { return state == State::Hold && timer >= params.holdDuration; }

	// --- プリセット ---
	// 戦闘開始（タイトル→ゲーム・起動時）。デフォルト
	static constexpr TransitionParams kBattleStartParams = { { 0.0f, 0.0f, 0.0f, 1.0f }, 0.4f, 0.15f, 0.6f, TransitionStyle::Tiles, 1.0f };
	// リトライ用。色は呼び出し側で勝敗色に上書きする想定
	static constexpr TransitionParams kRetryParams = { { 0.0f, 0.0f, 0.0f, 1.0f }, 0.45f, 0.15f, 0.45f };
	// 終了用（明転は事実上使われない）
	static constexpr TransitionParams kQuitParams = { { 0.0f, 0.0f, 0.0f, 1.0f }, 0.4f, 0.1f, 0.4f };
	// ゲーム→タイトルへ戻る用（ESCの確認からEnter）
	static constexpr TransitionParams kReturnTitleParams = { { 0.0f, 0.0f, 0.0f, 1.0f }, 0.4f, 0.1f, 0.4f };

private:
	// 状態と経過時間から進み具合を求める（線形。暗転:0→1 / Hold:1 / 明転:1→0）
	float CalcProgress() const;
	// 進み具合をスタイルごとの見た目へ反映する（見た目の差し替え口）
	void UpdateVisual();
	// グリッチ強度を計算してPostProcessへ送る（glitchStrength=0の遷移・終了時も0を送って残留を防ぐ）
	void UpdateGlitch(float progress);
	void UpdateFadeVisual(float progress);
	void UpdateTilesVisual(float progress);
	// タイルの遅延値を作り直す（遷移開始ごと）
	void RegenerateTileDelays();
	// 1枚のタイルの見た目を設定する
	void ApplyTile(size_t index, float local, bool fadingOut);

	std::unique_ptr<Sprite> overlay = nullptr; // 全画面の黒
	std::vector<std::unique_ptr<Sprite>> tiles; // タイルディゾルブ用（行優先）
	std::vector<float> tileScales;              // 各タイルの現在スケール（0なら描画スキップ）
	std::vector<float> tileDelays;             // 各タイルの遅延値(0～1)
	float tileWidth = 0.0f;
	float tileHeight = 0.0f;
	State state = State::None;
	float timer = 0.0f;
	float glitchTime = 0.0f; // グリッチの乱数シード用。遷移開始から毎フレーム加算（Holdでも進む）
	TransitionParams params = kBattleStartParams; // 現在の遷移設定

	static constexpr float kDeltaTime = 1.0f / 60.0f;

	// --- タイルディゾルブ ---
	static constexpr int kTileColumns = 16;
	static constexpr int kTileRows = 9;
	static constexpr float kTileSweepWeight = 0.4f;  // 遅延のうち中心→外側の偏りが占める割合
	static constexpr float kTileSpread = 0.6f;       // 遅延が進行度に占める幅
	static constexpr float kTileAccentPhase = 0.5f;  // この進行度まではアクセント色
	static constexpr Vector4 kTileAccentColor = { 0.2f, 0.9f, 1.0f, 1.0f };
};
