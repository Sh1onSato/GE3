#include "SceneFactory.h"
#include "TitleScene.h"
#include "GameScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName) {
    BaseScene* newScene = nullptr;

    if (sceneName == "TITLE") {
        newScene = new TitleScene();
    }
    else if (sceneName == "GAME") {
        newScene = new GameScene();
    }

    return newScene;
}
