#pragma once
#include "BaseScene.h"
#include <memory>

/// <summary>
/// シーン管理クラス
/// </summary>
class SceneManager {
public:
    // 更新
    void Update();
    // 描画
    void Draw();

    // シーンの切り替え予約
    // ※実際の切り替えは次のフレームの最初に行われる
    void ChangeScene(BaseScene* nextScene);

    // 終了処理
    void Finalize();

private:
    // 現在のシーン
    std::unique_ptr<BaseScene> currentScene = nullptr;
    // 次のシーン（遷移予約用）
    std::unique_ptr<BaseScene> nextScene = nullptr;
};
