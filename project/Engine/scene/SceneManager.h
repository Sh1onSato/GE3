#pragma once
#include "BaseScene.h"
#include "AbstractSceneFactory.h"
#include "SceneTransition.h"
#include <functional>
#include <memory>
#include <string>

/// <summary>
/// シーン管理クラス
/// </summary>
class SceneManager {
public:
    // 初期化（SpriteCommon生成後に呼ぶ。遷移演出を生成する）
    void Initialize();
    // 更新
    void Update();
    // 描画
    void Draw();

    // シーンの切り替え予約
    void ChangeScene(BaseScene* nextScene);

    // シーンの切り替え予約（文字列指定）
    void ChangeScene(const std::string& sceneName);

    // 起動直後の最初のシーン用：シーンを予約し、真っ暗から明転させる
    void ChangeSceneWithFadeIn(const std::string& sceneName);

    // フェード付きシーン遷移の要求（暗転→差し替え→明転）。遷移中は無視する
    void RequestTransition(const std::string& sceneName, const TransitionParams& params = SceneTransition::kBattleStartParams);

    // シーンを変えない暗転の要求。真っ暗になったフレームでonBlackを1回呼び、その後明転する。遷移中は無視する
    void RequestFade(const TransitionParams& params, std::function<void()> onBlack);

    // シーンファクトリーのセット
    void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory = factory; }

    // 終了処理
    void Finalize();

private:
    // 現在のシーン
    std::unique_ptr<BaseScene> currentScene = nullptr;
    // 次のシーン（遷移予約用）
    std::unique_ptr<BaseScene> nextScene = nullptr;

    // シーンファクトリー
    AbstractSceneFactory* sceneFactory = nullptr;

    // 遷移演出（フェード）と、暗転完了後に切り替える遷移先
    std::unique_ptr<SceneTransition> transition = nullptr;
    std::string pendingSceneName;
    // 暗転が真っ暗になった時に1回だけ呼ぶ処理（RequestFade用）
    std::function<void()> onBlack = nullptr;
};
