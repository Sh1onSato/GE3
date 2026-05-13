#include "TitleScene.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "Framework.h"
#include "TextureManager.h"

void TitleScene::Initialize() {
    Framework* framework = Framework::GetInstance();
    
    // タイトル用のテクスチャがあれば読み込む（今はサンプルを使用）
    TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");

    titleSprite = new Sprite();
    titleSprite->Initialize(framework->GetSpriteCommon(), "Resources/uvChecker.png");
    titleSprite->SetPosition({ 320.0f, 180.0f });
    titleSprite->SetSize({ 640.0f, 360.0f });
}

void TitleScene::Update() {
    Framework* framework = Framework::GetInstance();

    // スペースキーが押されたらゲームシーンへ
    if (framework->GetInput()->TriggerKey(DIK_SPACE)) {
        sceneManager->ChangeScene(new GameScene());
    }

    titleSprite->Update();
}

void TitleScene::Draw() {
    Framework* framework = Framework::GetInstance();

    // スプライト描画
    framework->GetSpriteCommon()->PreDraw();
    titleSprite->Draw();
}

void TitleScene::Finalize() {
    delete titleSprite;
}
