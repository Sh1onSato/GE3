#include "ScreenEffectController.h"
#include "PostProcess.h"
#include <algorithm>

void ScreenEffectController::Initialize(PostProcess* newPostProcess) {
    postProcess = newPostProcess;
    ResetTransientEffects();
}

void ScreenEffectController::Update(float deltaSeconds) {
    grayscaleFlashTimer = (std::max)(0.0f, grayscaleFlashTimer - deltaSeconds);
    damageVignetteTimer = (std::max)(0.0f, damageVignetteTimer - deltaSeconds);
}

void ScreenEffectController::Apply() {
    if (!postProcess) {
        return;
    }
    PostEffectSettings& settings = postProcess->GetSettings();

    // --- ブルーム（常に ON。強さだけメニューで変えられる） ---
    settings.bloomEnabled = true;
    settings.bloomThreshold = kBloomThreshold;
    settings.bloomIntensity = bloomIntensity;
    settings.bloomRadius = kBloomRadius;

    // --- 画面全体のぼかし（ボックス優先） ---
    if (boxBlurEnabled) {
        settings.blur = boxBlurStrength;
        settings.blurType = BlurType::kBox;
    } else if (blurEnabled) {
        settings.blur = screenBlurStrength;
        settings.blurType = BlurType::kGaussian;
    } else {
        settings.blur = 0.0f;
        settings.blurType = BlurType::kGaussian;
    }

    // --- グレースケール / セピア ---
    // フラッシュは三角波（前半 0→1 で濃くなり、後半 1→0 で戻る）。手動の ON とは独立で、強い方を使う
    float grayscaleFlashRatio = 0.0f;
    if (grayscaleFlashTimer > 0.0f) {
        const float t = 1.0f - (grayscaleFlashTimer / kWarpGrayscaleFlashDuration); // 0（開始）→ 1（終了）
        grayscaleFlashRatio = (t < 0.5f) ? (t * 2.0f) : ((1.0f - t) * 2.0f);
    }
    const float grayscaleBlend = (std::max)(grayscaleEnabled ? 1.0f : 0.0f, grayscaleFlashRatio);
    settings.sepia = sepiaEnabled ? 1.0f : 0.0f;
    // セピアのときは、セピアを優先する（グレースケールにしない）
    settings.saturation = sepiaEnabled ? 1.0f : (1.0f - grayscaleBlend);

    // --- ヴィネット ---
    // 被弾の赤いヴィネットを優先し、無いときは手動の黒いヴィネット（エンジンの PostProcess は 1 つしか持たないため）
    const float damageRatio = damageVignetteTimer / kDamageVignetteDuration; // 1（被弾直後）→ 0
    const float damageIntensity = damageRatio * kDamageVignetteMaxBlend;
    if (damageIntensity > 0.0f) {
        settings.vignetteColor = kDamageVignetteColor;
        settings.vignetteRadius = kDamageVignetteRadius;
        settings.vignetteIntensity = damageIntensity;
    } else {
        settings.vignetteColor = { 0.0f, 0.0f, 0.0f };
        settings.vignetteRadius = vignetteRadius;
        settings.vignetteIntensity = vignetteEnabled ? vignetteIntensity : 0.0f;
    }

    // --- 遷移グリッチ ---
    settings.glitchIntensity = transitionGlitchIntensity;
    settings.glitchTime = transitionGlitchTime;
}
