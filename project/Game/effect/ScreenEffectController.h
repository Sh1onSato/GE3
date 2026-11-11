#pragma once
#include "Structs.h"

class PostProcess;

/// <summary>
/// 画面の効果（グレースケール・セピア・ぼかし・ヴィネット・ブルーム・遷移グリッチ）の「手動の ON/OFF」と「一瞬だけの演出」を持つ、ゲーム側のクラス。
/// MyGame が 1 つ持ち、毎フレーム Apply() で、エンジンの PostProcess の設定のうち自分が担当する項目だけに書き込む。
/// （エンジンの PostProcess はシーンを切り替えるたびに既定の設定へ戻るが、ここが毎フレーム書き直すので、手動の ON/OFF はシーンをまたいで保たれる）
///
/// 担当する項目: bloom*、blur・blurType、saturation、sepia、vignette*、glitch*。
/// それ以外（明るさ・コントラスト・色合い・SSAO など）には触らない。
///
/// 使い方（シーンから）:
///   MyGame::GetInstance()->GetScreenEffect()->ToggleGrayscale();
///   MyGame::GetInstance()->GetScreenEffect()->TriggerDamageVignette();
/// </summary>
class ScreenEffectController {
public:
    void Initialize(PostProcess* newPostProcess);

    // 一瞬だけの演出のタイマーを進める（MyGame::Update が毎フレーム 1 回。ポーズ中も進めるので unscaled の秒数を渡す）
    void Update(float deltaSeconds);
    // PostProcess の設定へ書き込む（MyGame::Update が、SceneManager の更新の後に毎フレーム 1 回）
    void Apply();

    // --- 手動の ON/OFF（キー入力・ポーズ中の調整メニューから） ---
    void ToggleGrayscale() { grayscaleEnabled = !grayscaleEnabled; }
    // セピアは、グレースケールと同時に ON のときセピアを優先する
    void ToggleSepia() { sepiaEnabled = !sepiaEnabled; }
    // 画面全体のぼかし。ガウスとボックスは別々に ON/OFF でき、両方 ON ならボックスを優先する
    void ToggleBlur() { blurEnabled = !blurEnabled; }
    void ToggleBoxBlur() { boxBlurEnabled = !boxBlurEnabled; }
    void ToggleVignette() { vignetteEnabled = !vignetteEnabled; }

    // --- 一瞬だけの演出 ---
    // ワープ通過時などに一瞬だけグレースケールを掛ける（手動のグレースケールとは独立。強い方を使う）
    void TriggerGrayscaleFlash() { grayscaleFlashTimer = kWarpGrayscaleFlashDuration; }
    // 被弾時の一瞬の赤いヴィネット
    void TriggerDamageVignette() { damageVignetteTimer = kDamageVignetteDuration; }
    // シーン遷移・開始演出のグリッチの強さ（0 = 無し、1 = 最大）と、乱数の種にする経過時間[秒]。演出の側が毎フレーム渡す
    void SetTransitionGlitch(float intensity, float time) {
        transitionGlitchIntensity = intensity;
        transitionGlitchTime = time;
    }
    // 一瞬だけの演出の状態（グレースケールフラッシュ・被弾ヴィネット・グリッチ）だけを初期値に戻す。
    // シーンの始めに前のシーンの演出が残らないよう、各シーンの Initialize から呼ぶ（手動の ON/OFF は戻さない）
    void ResetTransientEffects() {
        grayscaleFlashTimer = 0.0f;
        damageVignetteTimer = 0.0f;
        transitionGlitchIntensity = 0.0f;
        transitionGlitchTime = 0.0f;
    }

    // --- ポーズ中の調整メニュー（PauseTuningMenu）から値を読み書きするためのアクセサ ---
    float GetBloomIntensity() const { return bloomIntensity; }
    void SetBloomIntensity(float value) { bloomIntensity = value; }
    bool IsBlurEnabled() const { return blurEnabled; }
    float GetScreenBlurStrength() const { return screenBlurStrength; }
    void SetScreenBlurStrength(float value) { screenBlurStrength = value; }
    bool IsBoxBlurEnabled() const { return boxBlurEnabled; }
    float GetBoxBlurStrength() const { return boxBlurStrength; }
    void SetBoxBlurStrength(float value) { boxBlurStrength = value; }
    bool IsGrayscaleEnabled() const { return grayscaleEnabled; }
    bool IsSepiaEnabled() const { return sepiaEnabled; }
    bool IsVignetteEnabled() const { return vignetteEnabled; }
    float GetVignetteIntensity() const { return vignetteIntensity; }
    void SetVignetteIntensity(float value) { vignetteIntensity = value; }
    float GetVignetteRadius() const { return vignetteRadius; }
    void SetVignetteRadius(float value) { vignetteRadius = value; }

    // --- 演出の定数 ---
    static constexpr float kWarpGrayscaleFlashDuration = 0.25f; // ワープ通過のグレースケールフラッシュ（三角波：前半で濃く、後半で戻る）
    static constexpr float kDamageVignetteDuration = 0.4f;      // 被弾ヴィネットが消えるまでの時間（被弾直後が最大で、だんだん薄くなる）
    static constexpr float kDamageVignetteMaxBlend = 0.4f;      // 被弾ヴィネットの最大の濃さ（画面が完全な赤一色にならないように）
    static constexpr float kDamageVignetteRadius = 0.35f;       // 被弾ヴィネットの半径（中心側はこの外側から赤くなる）
    static constexpr Vector3 kDamageVignetteColor = { 0.6f, 0.0f, 0.0f };

    // ブルームの既定値（旧 GE3 に近い見た目。ブルームは常に ON）
    static constexpr float kBloomThreshold = 0.95f;
    static constexpr float kBloomRadius = 1.0f;

private:
    PostProcess* postProcess = nullptr; // 所有しない（Framework が持つ）

    // 手動の ON/OFF
    bool grayscaleEnabled = false;
    bool sepiaEnabled = false;
    bool blurEnabled = false;
    bool boxBlurEnabled = false;
    bool vignetteEnabled = false;

    // 調整値
    float bloomIntensity = 1.0f;
    float screenBlurStrength = 2.0f; // ガウスのぼかしの幅（画面のピクセル）
    float boxBlurStrength = 0.4f;    // ボックスのぼかしの幅。全部均等に平均するので、ガウスより小さい値で同じくらい効く
    float vignetteIntensity = 0.8f;  // 0 = 無し、1 = 端が真っ黒
    float vignetteRadius = 0.5f;     // これより中心側は暗くしない（0 = 中心から、1 = 端だけ）

    // 一瞬だけの演出の残り時間[秒]。0 で終わり
    float grayscaleFlashTimer = 0.0f;
    float damageVignetteTimer = 0.0f;

    // 遷移グリッチ
    float transitionGlitchIntensity = 0.0f;
    float transitionGlitchTime = 0.0f;
};
