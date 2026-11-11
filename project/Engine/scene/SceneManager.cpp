#include "SceneManager.h"
#include "AudioManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "GameTime.h"
#include "Screen.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Framework.h"
#include "DebugDraw.h"
#include "Tween.h"
#include <algorithm>

SceneManager::SceneManager() = default;
// Spriteの完全な型が必要なため、デストラクタは.cppで定義する
SceneManager::~SceneManager() = default;

void SceneManager::Initialize(SpriteCommon* common) {
    spriteCommon = common;
    fadeSprite = std::make_unique<Sprite>();
    fadeSprite->Initialize(spriteCommon, "white");
    fadeSprite->SetPosition({ 0.0f, 0.0f });
    fadeSprite->SetSize(Screen::GetGameSize());
}

void SceneManager::SwapScene() {
    // 旧シーンの終了
    if (currentScene) {
        currentScene->Finalize();
    }

    // シーンの差し替え（ここで旧シーンが消え、旧シーンの Sprite などが持っていた画像の使用数が減る）
    currentScene = std::move(nextScene);
    nextScene = nullptr;

    // 旧シーンまでに読んだモデルと画像のうち、新シーンの Initialize で使われなかった物を次のフレームの始めに解放する
    ModelManager::GetInstance()->OnSceneChanged();
    TextureManager::GetInstance()->OnSceneChanged();
    // 旧シーンで積んだ確認用の線（秒数付きの物）を残さない。旧シーンのカメラを指したままにもしない
    DebugDraw::Clear();
    DebugDraw::SetCamera(nullptr);
    // 旧シーンで変えた画面の効果を、既定（MyGame で決めた物）に戻す
    Framework::GetInstance()->GetPostProcess()->ResetToDefault();
    // 反響は場所ごとの物なので、次のシーンに持ち越さない
    AudioManager::GetInstance()->SetReverb(ReverbPreset::kOff);
    // 旧シーンの物を動かしている Tween を止める（消えた物に書き込まないように）。ヒットストップも持ち越さない
    Tweens::KillAll();
    Time::CancelHitStop();

    if (currentScene) {
        // 次のシーンに自分（マネージャ）を教える
        currentScene->SetSceneManager(this);
        // 新シーンの初期化
        currentScene->Initialize();
    }
}

bool SceneManager::UpdateTransition() {
    if (!transition) {
        return false;
    }
    transition->Update();
    if (transition->IsReadyToSwitch()) {
        // 真っ暗になったので遷移先を予約し、明転へ進める（実際の差し替えはこの後の nextScene の処理で行う）
        if (!pendingSceneName.empty()) {
            if (sceneFactory) {
                nextScene.reset(sceneFactory->CreateScene(pendingSceneName));
            }
            pendingSceneName.clear();
        }
        // シーンを変えない暗転の場合は、真っ暗なこのフレームで 1 回だけ呼ぶ
        if (onBlack) {
            auto callback = std::move(onBlack);
            onBlack = nullptr;
            callback();
        }
        transition->BeginFadeIn();
        return false;
    }
    // 暗転中はシーンを止める（入力も無効）
    return transition->IsFadingOut();
}

void SceneManager::Update() {
    // --- ゲーム独自の遷移演出（あれば。暗転中はシーンを止める） ---
    if (UpdateTransition()) {
        return;
    }

    // --- フェードの進行（ポーズ中のtimeScaleに影響されないようunscaledで進める） ---
    if (fadeState != FadeState::kNone) {
        fadeTimer += Time::GetUnscaledDeltaTime();
        float t = fadeDuration > 0.0f ? (std::min)(1.0f, fadeTimer / fadeDuration) : 1.0f;

        if (fadeState == FadeState::kFadeOut) {
            fadeAlpha = t;
            if (t >= 1.0f) {
                // 真っ暗になった所でシーンを切り替え（RequestFade ならその処理を呼び）、明転に移る
                if (sceneFactory && !pendingSceneName.empty()) {
                    nextScene.reset(sceneFactory->CreateScene(pendingSceneName));
                }
                pendingSceneName.clear();
                if (onBlack) {
                    auto callback = std::move(onBlack);
                    onBlack = nullptr;
                    callback();
                }
                fadeState = FadeState::kFadeIn;
                fadeTimer = 0.0f;
            }
        } else {
            fadeAlpha = 1.0f - t;
            if (t >= 1.0f) {
                fadeState = FadeState::kNone;
                fadeAlpha = 0.0f;
            }
        }
    }

    // シーン切り替え予約がある場合
    if (nextScene) {
        SwapScene();
    }

    // 現在のシーンを更新
    if (currentScene) {
        currentScene->Update();
    }

    if (fadeSprite && fadeAlpha > 0.0f) {
        // ゲーム座標のサイズはウィンドウサイズや表示モードで変わるので、毎回ゲーム画面全体を覆うように合わせる
        fadeSprite->SetPosition({ 0.0f, 0.0f });
        fadeSprite->SetSize(Screen::GetGameSize());
        fadeSprite->SetColor({ fadeColor.x, fadeColor.y, fadeColor.z, fadeColor.w * fadeAlpha });
        fadeSprite->Update();
    }
}

void SceneManager::Draw() {
    // 現在のシーンを描画
    if (currentScene) {
        currentScene->Draw();
    }
    // 確認用の 3D の線（シーンの深度で物に隠れるように、3D と同じ描画先に描く）
    DebugDraw::Flush3D();
    // 内部解像度を下げて描いた 3D を画面に引き伸ばし、描画先をバックバッファに戻す（UI はこの後に等倍で描く）
    Framework::GetInstance()->GetRenderScaler()->EndScene();
    // シーンが積んだスプライトをレイヤー順に描画する
    if (spriteCommon) {
        spriteCommon->Flush();
    }
    // 確認用の 2D の線（UI より手前、フェードより奥）
    DebugDraw::Flush2D();

    // ゲーム独自の遷移演出（フェードより奥、UI より手前。レイヤーに関係なく必ず UI の上に重なる）
    if (transition && transition->IsActive() && spriteCommon) {
        transition->Draw();
        spriteCommon->Flush();
    }

    // フェード用の板を最前面に描画
    // （シーンのFlushとは別に後からFlushするので、レイヤー値に関係なく必ず一番手前になる）
    if (fadeSprite && spriteCommon && fadeAlpha > 0.0f) {
        fadeSprite->Draw();
        spriteCommon->Flush();
    }
}

void SceneManager::ChangeScene(BaseScene* nextScene) {
    // 次のシーンを予約
    this->nextScene.reset(nextScene);
}

void SceneManager::ChangeScene(const std::string& sceneName, float fadeSeconds, const Vector4& color) {
    if (!sceneFactory || IsTransitioning()) {
        return;
    }

    if (fadeSeconds <= 0.0f || !fadeSprite) {
        // ファクトリーを使ってシーンを生成し、予約する
        ChangeScene(sceneFactory->CreateScene(sceneName));
        return;
    }

    pendingSceneName = sceneName;
    fadeDuration = fadeSeconds;
    fadeTimer = 0.0f;
    fadeColor = color;
    fadeState = FadeState::kFadeOut;
}


void SceneManager::ChangeSceneWithTransition(const std::string& sceneName) {
    if (!sceneFactory || IsTransitioning()) {
        return;
    }
    if (transition) {
        pendingSceneName = sceneName;
        onBlack = nullptr;
        transition->Start();
        return;
    }
    // 演出が無いときは内蔵のフェード
    ChangeScene(sceneName, kFallbackFadeSeconds);
}

void SceneManager::StartFromBlack(const std::string& sceneName) {
    if (!sceneFactory) {
        return;
    }
    ChangeScene(sceneFactory->CreateScene(sceneName));
    if (transition) {
        transition->StartFromBlack();
    } else if (fadeSprite) {
        // 内蔵のフェードで、真っ暗から明転する
        fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        fadeDuration = kFallbackFadeSeconds;
        fadeTimer = 0.0f;
        fadeAlpha = 1.0f;
        fadeState = FadeState::kFadeIn;
    }
}

void SceneManager::RequestFade(std::function<void()> onBlackCallback) {
    if (IsTransitioning()) {
        return;
    }
    pendingSceneName.clear();
    onBlack = std::move(onBlackCallback);
    if (transition) {
        transition->Start();
    } else if (fadeSprite) {
        fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        fadeDuration = kFallbackFadeSeconds;
        fadeTimer = 0.0f;
        fadeState = FadeState::kFadeOut;
    } else if (onBlack) {
        // 描画の準備が無いときは暗転せずにすぐ呼ぶ
        auto callback = std::move(onBlack);
        onBlack = nullptr;
        callback();
    }
}

void SceneManager::Finalize() {
    if (currentScene) {
        currentScene->Finalize();
        currentScene.reset();
    }
    nextScene.reset();
}
