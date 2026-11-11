#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include "ISceneTransition.h"
#include "Structs.h"
#include <functional>
#include <memory>
#include <string>

class Sprite;
class SpriteCommon;

/// <summary>
/// シーン管理クラス
/// </summary>
class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    // フェード描画用の初期化（Framework::Initialize()から呼ばれる）
    void Initialize(SpriteCommon* spriteCommon);

    // 更新
    void Update();
    // 描画（シーンの描画後、フェード中なら画面全体を覆う板を重ねる）
    void Draw();

    // シーンの切り替え予約（次のUpdateで即切り替え）
    void ChangeScene(BaseScene* nextScene);

    // シーンの切り替え予約（文字列指定）
    // fadeSeconds > 0 なら、fadeSeconds秒かけて暗転 → 切り替え → fadeSeconds秒かけて明転する。
    // 遷移中に呼ばれた切り替え要求は無視する（決定ボタン連打で二重遷移しないように）
    void ChangeScene(const std::string& sceneName, float fadeSeconds = 0.0f, const Vector4& fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f });

    // --- ゲーム独自の遷移演出（任意） ---
    // 演出を渡すと、下の ChangeSceneWithTransition / StartFromBlack / RequestFade がその演出で「暗転 → 差し替え → 明転」を行う。
    // 所有しない（ゲーム側が生きている間は有効なまま持っておく）。nullptr なら内蔵のフェードで動く（既定）
    void SetTransition(ISceneTransition* newTransition) { transition = newTransition; }
    ISceneTransition* GetTransition() const { return transition; }

    // 演出つきのシーン切り替え（暗転 → 差し替え → 明転）。演出の設定は、渡した演出の側で先に済ませておく。
    // 演出が無いときは内蔵のフェード（黒、kFallbackFadeSeconds 秒）。遷移中の要求は無視する
    void ChangeSceneWithTransition(const std::string& sceneName);
    // 起動直後の最初のシーン用: シーンを予約し、真っ暗な状態から明転する
    void StartFromBlack(const std::string& sceneName);
    // シーンを変えない暗転。真っ暗になったフレームで onBlack を 1 回呼び、その後明転する（リトライ・やり直し用）。遷移中は無視する
    void RequestFade(std::function<void()> onBlack);

    // フェード中（暗転・明転のどちらか。ゲーム独自の遷移演出を含む）か。遷移中は入力を受け付けない等の判断に使う
    bool IsTransitioning() const { return fadeState != FadeState::kNone || (transition && transition->IsActive()); }

    // シーンファクトリーのセット
    void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory = factory; }

    // 終了処理
    void Finalize();

private:
    enum class FadeState {
        kNone,
        kFadeOut, // 暗転中（終わったらシーンを切り替える）
        kFadeIn,  // 明転中
    };

    void SwapScene();
    // ゲーム独自の遷移演出を 1 フレーム進める。暗転中でシーンを止めるべきなら true を返す
    bool UpdateTransition();

    // 演出が無いときの内蔵フェードの長さ[秒]
    static constexpr float kFallbackFadeSeconds = 0.4f;

    // 現在のシーン
    std::unique_ptr<BaseScene> currentScene = nullptr;
    // 次のシーン（遷移予約用）
    std::unique_ptr<BaseScene> nextScene = nullptr;

    // シーンファクトリー
    AbstractSceneFactory* sceneFactory = nullptr;

    // ゲーム独自の遷移演出（所有しない）と、暗転が終わったら 1 回だけ呼ぶ処理（RequestFade 用）
    ISceneTransition* transition = nullptr;
    std::function<void()> onBlack = nullptr;

    // --- フェード ---
    SpriteCommon* spriteCommon = nullptr;
    std::unique_ptr<Sprite> fadeSprite;
    FadeState fadeState = FadeState::kNone;
    std::string pendingSceneName;
    float fadeDuration = 0.0f;
    float fadeTimer = 0.0f;
    Vector4 fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float fadeAlpha = 0.0f;
};
