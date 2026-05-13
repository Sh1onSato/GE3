#include "GameScene.h"
#include "Framework.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "externals/imgui/imgui.h"

void GameScene::Initialize() {
    Framework* framework = Framework::GetInstance();

    // 1. テクスチャを先にロードしておく (インデックス取得を確実にするため)
    TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
    TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");

    // 2. カメラの生成と登録
    std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();
    CameraManager::GetInstance()->AddCamera("MainCamera", std::move(newCamera));
    camera = CameraManager::GetInstance()->GetActiveCamera();

    // 3. モデルの生成と初期化
    model = new Model();
    model->Initialize(framework->GetDxCommon(), "Resources", "plane.obj");

    // 4. 3Dオブジェクトの生成と設定
    object3d = new Object3d();
    object3d->Initialize(framework->GetObject3dCommon());
    object3d->SetModel(model);

    object3d2 = new Object3d();
    object3d2->Initialize(framework->GetObject3dCommon());
    object3d2->SetModel(model);
    object3d2->SetTranslate({ 2.0f, 0.0f, 0.0f });
    object3d2->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    // 5. スプライトの生成
    uvChecker = new Sprite();
    uvChecker->Initialize(framework->GetSpriteCommon(), "Resources/uvChecker.png");
    monsterBall = new Sprite();
    monsterBall->Initialize(framework->GetSpriteCommon(), "Resources/monsterBall.png");

    uvChecker->SetPosition({ 0.0f, 0.0f });
    monsterBall->SetPosition({ 640.0f, 360.0f });
    monsterBall->SetSize({ 128.0f, 128.0f });

    // 6. パーティクルの初期化 (テクスチャがロード済みであることを確認してから)
    particleManager = new ParticleManager();
    uint32_t texIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("Resources/monsterBall.png");
    particleManager->Initialize(framework->GetParticleCommon(), texIndex);

    particleEmitter = new ParticleEmitter();
    ParticleEmitter::EmitterSetting setting;
    setting.position = { 0.0f, 0.0f, 0.0f };
    setting.minVelocity = { -0.1f, 0.1f, -0.1f };
    setting.maxVelocity = { 0.1f, 0.3f, 0.1f };
    setting.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    setting.lifeTime = 1.0f;
    setting.count = 2;
    setting.frequency = 0.1f;
    particleEmitter->Initialize("TestEmitter", particleManager, setting);
}

void GameScene::Update() {
    uvChecker->Update();
    monsterBall->Update();
    camera->Update();
    object3d->Update();
    object3d2->Update();
    particleManager->Update();
    particleEmitter->Update();

    ImGui::Begin("DebugManager");
    if (ImGui::BeginTabBar("MainTabBar")) {
        if (ImGui::BeginTabItem("UV Checker")) { uvChecker->ImGui(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Monster Ball")) { monsterBall->ImGui(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("3D Object 1")) { object3d->ImGui(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("3D Object 2")) { object3d2->ImGui(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Camera")) { camera->ImGui(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Particle Emitter")) {
            ParticleEmitter::EmitterSetting setting = particleEmitter->GetSetting();
            ImGui::DragFloat3("Position", &setting.position.x, 0.1f);
            ImGui::DragFloat3("Min Velocity", &setting.minVelocity.x, 0.01f);
            ImGui::DragFloat3("Max Velocity", &setting.maxVelocity.x, 0.01f);
            ImGui::ColorEdit4("Color", &setting.color.x);
            ImGui::DragFloat("LifeTime", &setting.lifeTime, 0.1f, 0.0f, 10.0f);
            int count = (int)setting.count;
            ImGui::DragInt("Count", &count, 1, 1, 100);
            setting.count = (uint32_t)count;
            ImGui::DragFloat("Frequency", &setting.frequency, 0.01f, 0.01f, 1.0f);
            
            particleEmitter->SetSetting(setting);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void GameScene::Draw() {
    Framework* framework = Framework::GetInstance();

    framework->GetObject3dCommon()->PreDraw();
    object3d->Draw();
    object3d2->Draw();

    framework->GetParticleCommon()->PreDraw();
    particleManager->Draw();

    framework->GetSpriteCommon()->PreDraw();
    uvChecker->Draw();
    monsterBall->Draw();
}

void GameScene::Finalize() {
    delete object3d;
    delete object3d2;
    delete monsterBall;
    delete uvChecker;
    delete model;
    delete particleEmitter;
    delete particleManager;
}
