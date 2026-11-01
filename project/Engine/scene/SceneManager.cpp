#include "SceneManager.h"

void SceneManager::Initialize() {
    transition = std::make_unique<SceneTransition>();
    transition->Initialize();
}

void SceneManager::Update() {
    // 遷移演出の更新
    if (transition) {
        transition->Update();

        if (transition->IsReadyToSwitch()) {
            // 真っ暗になったので遷移先を予約し、明転へ進める（実際の差し替えは下の既存処理で行う）
            if (!pendingSceneName.empty()) {
                ChangeScene(pendingSceneName);
                pendingSceneName.clear();
            }
            // シーンを変えない暗転の場合は、真っ暗なこのフレームで1回だけコールバックを呼ぶ
            if (onBlack) {
                auto callback = std::move(onBlack);
                onBlack = nullptr;
                callback();
            }
            transition->BeginFadeIn();
        } else if (transition->IsFadingOut()) {
            // 暗転中はシーンを止める（入力も無効）
            return;
        }
    }

    // シーン切り替え予約がある場合
    if (nextScene) {
        // 旧シーンの終了
        if (currentScene) {
            currentScene->Finalize();
        }

        // シーンの差し替え
        currentScene = std::move(nextScene);
        nextScene = nullptr;

        // 次のシーンに自分（マネージャ）を教える
        currentScene->SetSceneManager(this);
        // 新シーンの初期化
        currentScene->Initialize();

        if (currentScene) {
            currentScene->Update();
        }
        return;
    }

    // 現在のシーンを更新
    if (currentScene) {
        currentScene->Update();
    }
}

void SceneManager::Draw() {
    // 現在のシーンを描画
    if (currentScene) {
        currentScene->Draw();
    }
    // 遷移用の黒幕は常に最前面
    if (transition) {
        transition->Draw();
    }
}

void SceneManager::ChangeSceneWithFadeIn(const std::string& sceneName) {
    ChangeScene(sceneName);
    if (transition) {
        transition->StartFromBlack(SceneTransition::kBattleStartParams);
    }
}

void SceneManager::RequestTransition(const std::string& sceneName, const TransitionParams& params) {
    // 多重遷移防止
    if (!transition || transition->IsActive()) {
        return;
    }
    pendingSceneName = sceneName;
    transition->Start(params);
}

void SceneManager::RequestFade(const TransitionParams& params, std::function<void()> onBlack) {
    // 多重遷移防止
    if (!transition || transition->IsActive()) {
        return;
    }
    pendingSceneName.clear();
    this->onBlack = std::move(onBlack);
    transition->Start(params);
}

void SceneManager::ChangeScene(BaseScene* nextScene) {
    // 次のシーンを予約
    this->nextScene.reset(nextScene);
}

void SceneManager::ChangeScene(const std::string& sceneName) {
    if (sceneFactory) {
        // ファクトリーを使ってシーンを生成し、予約する
        BaseScene* next = sceneFactory->CreateScene(sceneName);
        ChangeScene(next);
    }
}


void SceneManager::Finalize() {
    if (currentScene) {
        currentScene->Finalize();
        currentScene.reset();
    }
    transition.reset();
}
