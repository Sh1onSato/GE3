#include "GameScene.h"
#include "Framework.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "externals/imgui/imgui.h"
#include <algorithm>

void GameScene::Initialize() {
    Framework* framework = Framework::GetInstance();

    TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
    TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");
    // TextureManager::GetInstance()->LoadTexture("Resources/white1x1.png"); // 一時コメントアウト

    reticle = std::make_unique<Sprite>();
    reticle->Initialize(framework->GetSpriteCommon(), "Resources/uvChecker.png"); // 代わりのテクスチャ
    // 画面中央に配置 (1280x720)
    reticle->SetPosition({ (float)WinApp::KclientWidth / 2.0f, (float)WinApp::KclientHeight / 2.0f });
    // サイズを小さく (6x6ピクセル程度)
    reticle->SetSize({ 6.0f, 6.0f });
    // 中心点をずらして真ん中に
    reticle->SetAnchorPoint({ 0.5f, 0.5f });
    // 目立つように赤色にする
    reticle->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();
    CameraManager::GetInstance()->AddCamera("MainCamera", std::move(newCamera));
    camera = CameraManager::GetInstance()->GetActiveCamera();
    
    playerPos = { 0.0f, 0.0f, -10.0f };
    camera->SetTranslate({ playerPos.x, playerPos.y + eyeHeight, playerPos.z });

    cubeModel = std::make_unique<Model>();
    cubeModel->Initialize(framework->GetDxCommon(), "Resources/cube", "cube.obj");

    skybox = std::make_unique<Skybox>();
    skybox->Initialize(framework->GetSkyboxCommon(), "Resources/skybox/sky_cubemap.dds");
    skybox->SetModel(cubeModel.get());

    floor = std::make_unique<Object3d>();
    floor->Initialize(framework->GetObject3dCommon());
    floor->SetModel(cubeModel.get());
    floor->SetScale({ 20.0f, 1.0f, 20.0f });
    floor->SetTranslate({ 0.0f, -1.0f, 0.0f });
    floor->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });

    struct WallData { Vector3 pos; Vector3 scale; };
    std::vector<WallData> wallDatas = {
        { { 0.0f, 1.0f, 20.0f }, { 20.0f, 2.0f, 1.0f } },
        { { 0.0f, 1.0f, -20.0f }, { 20.0f, 2.0f, 1.0f } },
        { { 20.0f, 1.0f, 0.0f }, { 1.0f, 2.0f, 20.0f } },
        { { -20.0f, 1.0f, 0.0f }, { 1.0f, 2.0f, 20.0f } },
        { { 5.0f, 1.0f, 5.0f }, { 2.0f, 2.0f, 2.0f } },
        { { -8.0f, 1.0f, -5.0f }, { 3.0f, 2.0f, 1.0f } },
        { { 0.0f, 1.0f, 0.0f }, { 1.0f, 2.0f, 1.0f } },
    };

    for (const auto& data : wallDatas) {
        auto wall = std::make_unique<Object3d>();
        wall->Initialize(framework->GetObject3dCommon());
        wall->SetModel(cubeModel.get());
        wall->SetTranslate(data.pos);
        wall->SetScale(data.scale);
        walls.push_back(std::move(wall));
    }

    // 初期状態はプレイ中（マウス非表示・固定）
    isLookPaused = false;
    framework->GetWinApp()->ShowCursor(false);
    framework->GetWinApp()->SetClipCursor(true);
}

bool GameScene::CheckCollision(const Vector3& pos, float* outMaxY) {
    Vector3 center = { pos.x, pos.y + collisionRadius, pos.z };
    std::vector<Object3d*> colliders;
    if (floor) colliders.push_back(floor.get());
    for (auto& w : walls) colliders.push_back(w.get());

    bool isHit = false;
    float highestY = -10000.0f; // 十分低い値

    for (Object3d* wall : colliders) {
        if (!wall) continue;
        Transform wTransform = wall->GetTransform();
        Vector3 wPos = wTransform.translate;
        Vector3 wScale = wTransform.scale;

        float minX = wPos.x - wScale.x;
        float maxX = wPos.x + wScale.x;
        float minY = wPos.y - wScale.y;
        float maxY = wPos.y + wScale.y;
        float minZ = wPos.z - wScale.z;
        float maxZ = wPos.z + wScale.z;

        float closestX = (std::max)(minX, (std::min)(center.x, maxX));
        float closestY = (std::max)(minY, (std::min)(center.y, maxY));
        float closestZ = (std::max)(minZ, (std::min)(center.z, maxZ));

        float dx = center.x - closestX;
        float dy = center.y - closestY;
        float dz = center.z - closestZ;
        float distanceSq = dx * dx + dy * dy + dz * dz;

        if (distanceSq <= collisionRadius * collisionRadius) {
            isHit = true;
            if (maxY > highestY) {
                highestY = maxY;
            }
        }
    }

    if (outMaxY) *outMaxY = highestY;
    return isHit;
}

void GameScene::Update() {
    Framework* framework = Framework::GetInstance();
    WinApp* winApp = framework->GetWinApp();
    Input* input = framework->GetInput();
    Transform& cameraTransform = camera->GetTransform();

    // --- マウス操作（プレイ中のみ） ---
    if (!isLookPaused) {
        Input::MouseMove mouseMove = input->GetMouseMove();
        cameraTransform.rotate.y += (float)mouseMove.lX * mouseSensitivity;
        cameraTransform.rotate.x += (float)mouseMove.lY * mouseSensitivity;

        // マウスを中央に固定し続ける
        POINT center = { WinApp::KclientWidth / 2, WinApp::KclientHeight / 2 };
        ClientToScreen(winApp->GetHwnd(), &center);
        SetCursorPos(center.x, center.y);
    }

    // --- ポーズ切り替え（Pキー） ---
    if (input->TriggerKey(DIK_P)) {
        isLookPaused = !isLookPaused;
        
        // 状態に合わせてマウスの表示・固定を切り替え
        if (isLookPaused) {
            winApp->ShowCursor(true);
            winApp->SetClipCursor(false);
        } else {
            winApp->ShowCursor(false);
            winApp->SetClipCursor(true);
        }
    }

    // 移動方向の計算
    const float moveSpeed = 0.15f;
    Vector3 moveDir = { 0, 0, 0 };
    Vector3 forward = { std::sin(cameraTransform.rotate.y), 0.0f, std::cos(cameraTransform.rotate.y) };
    Vector3 right = { std::cos(cameraTransform.rotate.y), 0.0f, -std::sin(cameraTransform.rotate.y) };

    if (input->PushKey(DIK_W)) { moveDir += forward; }
    if (input->PushKey(DIK_S)) { moveDir -= forward; }
    if (input->PushKey(DIK_A)) { moveDir -= right; }
    if (input->PushKey(DIK_D)) { moveDir += right; }

    if (Calculation::Length(moveDir) > 0) {
        moveDir = Calculation::Normalize(moveDir) * moveSpeed;
    }

    const float kGravity = -0.02f;
    const float kJumpWidth = 0.38f;
    velocity.y += kGravity;

    if (isOnGround && input->TriggerKey(DIK_SPACE)) {
        velocity.y = kJumpWidth;
        isOnGround = false;
    }

    // 移動と判定
    const float kStepHeight = 0.4f; // 乗り越えられる段差の高さ
    const float kGroundOffset = 0.05f; // 床との判定を避けるためのオフセット

    // X軸の移動と段差判定
    Vector3 nextPosX = playerPos;
    nextPosX.x += moveDir.x;
    // 判定時は少し浮かせる（地面を壁と誤認しないため）
    Vector3 checkPosX = nextPosX;
    checkPosX.y += kGroundOffset;
    if (!CheckCollision(checkPosX)) {
        playerPos.x = nextPosX.x;
    } else {
        // 段差乗り越え試行
        Vector3 nextPosXStep = nextPosX;
        nextPosXStep.y += kStepHeight;
        if (!CheckCollision(nextPosXStep)) {
            playerPos.x = nextPosX.x;
            playerPos.y += kStepHeight;
        }
    }

    // Z軸の移動と段差判定
    Vector3 nextPosZ = playerPos;
    nextPosZ.z += moveDir.z;
    // 判定時は少し浮かせる
    Vector3 checkPosZ = nextPosZ;
    checkPosZ.y += kGroundOffset;
    if (!CheckCollision(checkPosZ)) {
        playerPos.z = nextPosZ.z;
    } else {
        // 段差乗り越え試行
        Vector3 nextPosZStep = nextPosZ;
        nextPosZStep.y += kStepHeight;
        if (!CheckCollision(nextPosZStep)) {
            playerPos.z = nextPosZ.z;
            playerPos.y += kStepHeight;
        }
    }

    // Y軸の移動（重力・ジャンプ）
    isOnGround = false;
    Vector3 nextPosY = playerPos;
    nextPosY.y += velocity.y;
    float hitHeight = 0.0f;
    if (!CheckCollision(nextPosY, &hitHeight)) {
        playerPos.y = nextPosY.y;
    } else {
        // 衝突した場合
        if (velocity.y <= 0) { // 落下中または静止中なら接地
            isOnGround = true;
            // 接地時は急激なスナップを避け、めり込んでいる時だけ補正する
            if (playerPos.y < hitHeight) {
                playerPos.y = hitHeight;
            }
        }
        velocity.y = 0;
    }

    cameraTransform.translate = { playerPos.x, playerPos.y + eyeHeight, playerPos.z };

    skybox->Update();
    floor->Update();
    for (auto& wall : walls) { wall->Update(); }
    camera->Update();
    reticle->Update();

    if (isLookPaused) {
        ImGui::Begin("FPS Debug");
        if (ImGui::Button("RESUME GAME (P)")) {
            isLookPaused = !isLookPaused;
            winApp->ShowCursor(isLookPaused);
            winApp->SetClipCursor(!isLookPaused);
        }
        ImGui::SliderFloat("Sensitivity", &mouseSensitivity, 0.0001f, 0.01f, "%.4f");
        ImGui::SliderFloat("Eye Height", &eyeHeight, 0.1f, 3.0f, "%.1f");
        ImGui::Text("Pos: %.2f, %.2f, %.2f", playerPos.x, playerPos.y, playerPos.z);
        ImGui::End();
    }
}

void GameScene::Draw() {
    Framework* framework = Framework::GetInstance();

    // スカイボックスの描画
    framework->GetSkyboxCommon()->PreDraw();
    skybox->Draw();

    // 3Dオブジェクトの描画
    framework->GetObject3dCommon()->PreDraw();
    floor->Draw();
    for (auto& wall : walls) { wall->Draw(); }

    // UI（レティクル）の描画
    framework->GetSpriteCommon()->PreDraw();
    reticle->Draw();
}

void GameScene::Finalize() {
    // 終了時は必ずマウスを解放する
    Framework::GetInstance()->GetWinApp()->ShowCursor(true);
    Framework::GetInstance()->GetWinApp()->SetClipCursor(false);
}
