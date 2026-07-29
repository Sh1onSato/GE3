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
    TextureManager::GetInstance()->LoadTexture("Resources/syouzyuu1.png");
    
    // 内部生成された白テクスチャを取得してパーティクルを初期化
    uint32_t whiteTexIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("white");
    // 点・線・面を同時に出せるよう、描画タイプごとに別々のParticleManagerを用意する
    particleManagerLine = std::make_unique<ParticleManager>();
    particleManagerLine->Initialize(framework->GetParticleCommon(), whiteTexIndex, ParticleDrawType::kLine);

    particleManagerPoint = std::make_unique<ParticleManager>();
    particleManagerPoint->Initialize(framework->GetParticleCommon(), whiteTexIndex, ParticleDrawType::kPoint);

    particleManagerTriangle = std::make_unique<ParticleManager>();
    particleManagerTriangle->Initialize(framework->GetParticleCommon(), whiteTexIndex, ParticleDrawType::kTriangle);

    postProcess = std::make_unique<PostProcess>();
    postProcess->Initialize(framework->GetDxCommon(), framework->GetSrvManager());

    reticle = std::make_unique<Sprite>();
    reticle->Initialize(framework->GetSpriteCommon(), "Resources/syouzyuu1.png");
    // 画面中央に配置 (1280x720)
    reticle->SetPosition({ (float)WinApp::KclientWidth / 2.0f, (float)WinApp::KclientHeight / 2.0f });
    // サイズを少し大きく (12x12ピクセル程度)
    reticle->SetSize({ 40.0f, 40.0f });
    // 中心点をずらして真ん中に
    reticle->SetAnchorPoint({ 0.5f, 0.5f });
    // 赤い壁の上でも見えるように「緑色」にする
    reticle->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    // --- プレイヤーHPバー（画面左下、常時表示） ---
    hpBarBackground = std::make_unique<Sprite>();
    hpBarBackground->Initialize(framework->GetSpriteCommon(), "white");
    hpBarBackground->SetPosition(kHpBarPosition);
    hpBarBackground->SetSize({ kHpBarMaxWidth, kHpBarHeight });
    hpBarBackground->SetColor({ 0.1f, 0.1f, 0.1f, 0.8f });

    hpBarFill = std::make_unique<Sprite>();
    hpBarFill->Initialize(framework->GetSpriteCommon(), "white");
    hpBarFill->SetPosition(kHpBarPosition);
    hpBarFill->SetSize({ kHpBarMaxWidth, kHpBarHeight });
    hpBarFill->SetColor({ 0.85f, 0.15f, 0.15f, 1.0f });

    playerHpText.Initialize(framework->GetSpriteCommon(), 3); // 最大3桁("0"-"100")
    playerHpText.SetPosition(kHpTextPosition);
    playerHpText.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // --- ボスHPバー（画面上部中央、常時表示） ---
    bossHpBarBackground = std::make_unique<Sprite>();
    bossHpBarBackground->Initialize(framework->GetSpriteCommon(), "white");
    bossHpBarBackground->SetPosition(kBossHpBarPosition);
    bossHpBarBackground->SetSize({ kBossHpBarMaxWidth, kBossHpBarHeight });
    bossHpBarBackground->SetColor({ 0.1f, 0.1f, 0.1f, 0.8f });

    bossHpBarFill = std::make_unique<Sprite>();
    bossHpBarFill->Initialize(framework->GetSpriteCommon(), "white");
    bossHpBarFill->SetPosition(kBossHpBarPosition);
    bossHpBarFill->SetSize({ kBossHpBarMaxWidth, kBossHpBarHeight });
    bossHpBarFill->SetColor({ 0.6f, 0.05f, 0.5f, 1.0f });

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
    floor->SetScale({ 30.0f, 1.0f, 30.0f });
    floor->SetTranslate({ 0.0f, -1.0f, 0.0f });
    floor->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });

    struct WallData { Vector3 pos; Vector3 scale; };
    std::vector<WallData> wallDatas = {
        // --- 外壁（閉じた箱型アリーナの四方。±30に拡張し、1v1ボス戦を見据えた構成） ---
        // 上下移動(四隅の上段プラットフォーム高さ5.0＋ジャンプ最大加算高さ約3.61=最大到達約8.6)を考慮し、
        // 場外に出られないよう上端をy=15.0まで引き上げ（下端y=-1.0は床への埋め込みのため変更なし）
        { { 0.0f, 7.0f, 30.0f }, { 30.0f, 8.0f, 1.0f } },
        { { 0.0f, 7.0f, -30.0f }, { 30.0f, 8.0f, 1.0f } },
        { { 30.0f, 7.0f, 0.0f }, { 1.0f, 8.0f, 30.0f } },
        { { -30.0f, 7.0f, 0.0f }, { 1.0f, 8.0f, 30.0f } },
        // --- 中央のカバー（十字型に対称配置。中心は空けて見通しを確保しつつ、物陰に隠れられるようにする） ---
        { { 12.0f, 1.0f, 0.0f }, { 1.5f, 2.0f, 1.5f } },
        { { -12.0f, 1.0f, 0.0f }, { 1.5f, 2.0f, 1.5f } },
        { { 0.0f, 1.0f, 12.0f }, { 1.5f, 2.0f, 1.5f } },
        { { 0.0f, 1.0f, -12.0f }, { 1.5f, 2.0f, 1.5f } },
        // --- 四隅の2段プラットフォーム（上下を使った戦い用。ジャンプ1回で下段(高さ2.5)、
        //     さらにもう1回ジャンプで上段(高さ5.0)に登れる高低差にしている） ---
        { { 14.0f, 1.25f, 14.0f }, { 3.0f, 1.25f, 3.0f } },
        { { 14.0f, 3.75f, 14.0f }, { 1.5f, 1.25f, 1.5f } },
        { { -14.0f, 1.25f, 14.0f }, { 3.0f, 1.25f, 3.0f } },
        { { -14.0f, 3.75f, 14.0f }, { 1.5f, 1.25f, 1.5f } },
        { { 14.0f, 1.25f, -14.0f }, { 3.0f, 1.25f, 3.0f } },
        { { 14.0f, 3.75f, -14.0f }, { 1.5f, 1.25f, 1.5f } },
        { { -14.0f, 1.25f, -14.0f }, { 3.0f, 1.25f, 3.0f } },
        { { -14.0f, 3.75f, -14.0f }, { 1.5f, 1.25f, 1.5f } },
    };

    for (const auto& data : wallDatas) {
        auto wall = std::make_unique<Object3d>();
        wall->Initialize(framework->GetObject3dCommon());
        wall->SetModel(cubeModel.get());
        wall->SetTranslate(data.pos);
        wall->SetScale(data.scale);
        walls.push_back(std::move(wall));
    }

    // 敵キャラクター（第一段階：静的な的）を、既存の壁・障害物・プラットフォームと重ならない位置に配置する
    std::vector<Vector3> enemyPositions = {
        { 20.0f, 0.75f, 20.0f },
        { -20.0f, 0.75f, 20.0f },
        { 20.0f, 0.75f, -20.0f },
        { -20.0f, 0.75f, -20.0f },
    };
    for (const auto& pos : enemyPositions) {
        EnemyTarget enemy;
        enemy.object = std::make_unique<Object3d>();
        enemy.object->Initialize(framework->GetObject3dCommon());
        enemy.object->SetModel(cubeModel.get());
        enemy.object->SetTranslate(pos);
        enemy.object->SetScale({ 0.5f, 0.75f, 0.5f });
        enemy.object->SetColor({ 1.0f, 0.5f, 0.0f, 1.0f }); // オレンジ色で壁と区別する
        enemy.isActive = true;
        enemy.respawnTimer = 0.0f;
        enemies.push_back(std::move(enemy));
    }

    // ボス（1v1ボス戦フェーズ2：HP/UIの土台。移動・攻撃AIは次フェーズ）
    // 中央カバー(±12)・角プラットフォーム(±14)と被らない開けた位置に配置
    boss = std::make_unique<Object3d>();
    boss->Initialize(framework->GetObject3dCommon());
    boss->SetModel(cubeModel.get());
    // 底面が床(y=0)にちょうど乗るよう、bossPosの足元基準にkBossHalfHeightを足した高さに配置する
    // （前フェーズではy=1.5決め打ちで底面が1ユニット埋まっていたため、ここで修正）
    boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });
    boss->SetScale({ 2.0f, 2.5f, 2.0f });
    boss->SetColor({ 0.5f, 0.0f, 0.4f, 1.0f }); // 紫系でプレイヤー・的と区別する

    planeModel = std::make_unique<Model>();
    // plane.obj相当のXY平面(法線+Z、2x2サイズ)をコード内で生成し、外部リソースファイルへの依存をなくす
    std::vector<VertexData> decalQuadVertices = {
        { {  1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { {  1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { {  1.0f,  1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f,  1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    planeModel->InitializeFromVertices(framework->GetDxCommon(), decalQuadVertices);

    bulletDecals.reserve(kMaxBulletDecals);
    for (int i = 0; i < kMaxBulletDecals; ++i) {
        auto decal = std::make_unique<Object3d>();
        decal->Initialize(framework->GetObject3dCommon());
        decal->SetModel(planeModel.get());
        decal->SetScale({ 0.15f, 0.15f, 0.15f });
        decal->SetTranslate({ 0.0f, -1000.0f, 0.0f }); // 未使用分は画面外に隠しておく
        decal->SetColor({ 0.15f, 0.15f, 0.15f, 1.0f }); // 透過PNG未用意のため暗色プレースホルダー
        bulletDecals.push_back(std::move(decal));
    }

    // --- 視点武器（プレースホルダー：新規アセットは使わずcubeModelを流用） ---
    weaponBody = std::make_unique<Object3d>();
    weaponBody->Initialize(framework->GetObject3dCommon());
    weaponBody->SetModel(cubeModel.get());
    weaponBody->SetScale({ 0.08f, 0.06f, 0.20f }); // half-extent。全長40cm程度の本体
    weaponBody->SetColor({ 0.12f, 0.12f, 0.13f, 1.0f });

    weaponBarrel = std::make_unique<Object3d>();
    weaponBarrel->Initialize(framework->GetObject3dCommon());
    weaponBarrel->SetModel(cubeModel.get());
    weaponBarrel->SetScale({ 0.03f, 0.03f, 0.16f }); // 本体より細い銃身
    weaponBarrel->SetColor({ 0.05f, 0.05f, 0.05f, 1.0f });

    // --- 設定メニュー（ImGui非依存。Pキーでのポーズ中にキー操作で調整できる） ---
    {
        PostProcess* pp = postProcess.get();
        std::vector<SettingsMenu::Item> menuItems;

        menuItems.push_back({ { 1.0f, 0.6f, 0.1f, 1.0f }, false, 0.0f, 10.0f, 0.05f,
            [pp] { return pp->GetBloomIntensity(); }, [pp](float v) { pp->SetBloomIntensity(v); } });

        menuItems.push_back({ { 0.3f, 0.7f, 1.0f, 1.0f }, true, 0.0f, 1.0f, 0.0f,
            [pp] { return pp->IsBlurEnabled() ? 1.0f : 0.0f; },
            [pp](float v) { if ((v > 0.5f) != pp->IsBlurEnabled()) pp->ToggleBlur(); } });
        menuItems.push_back({ { 0.3f, 0.7f, 1.0f, 1.0f }, false, 0.0f, 10.0f, 0.05f,
            [pp] { return pp->GetScreenBlurStrength(); }, [pp](float v) { pp->SetScreenBlurStrength(v); } });

        menuItems.push_back({ { 0.2f, 0.3f, 0.8f, 1.0f }, true, 0.0f, 1.0f, 0.0f,
            [pp] { return pp->IsBoxBlurEnabled() ? 1.0f : 0.0f; },
            [pp](float v) { if ((v > 0.5f) != pp->IsBoxBlurEnabled()) pp->ToggleBoxBlur(); } });
        menuItems.push_back({ { 0.2f, 0.3f, 0.8f, 1.0f }, false, 0.0f, 5.0f, 0.02f,
            [pp] { return pp->GetBoxBlurStrength(); }, [pp](float v) { pp->SetBoxBlurStrength(v); } });

        menuItems.push_back({ { 0.6f, 0.6f, 0.6f, 1.0f }, true, 0.0f, 1.0f, 0.0f,
            [pp] { return pp->IsGrayscaleEnabled() ? 1.0f : 0.0f; },
            [pp](float v) { if ((v > 0.5f) != pp->IsGrayscaleEnabled()) pp->ToggleGrayscale(); } });

        menuItems.push_back({ { 0.55f, 0.35f, 0.15f, 1.0f }, true, 0.0f, 1.0f, 0.0f,
            [pp] { return pp->IsSepiaEnabled() ? 1.0f : 0.0f; },
            [pp](float v) { if ((v > 0.5f) != pp->IsSepiaEnabled()) pp->ToggleSepia(); } });

        menuItems.push_back({ { 0.5f, 0.1f, 0.5f, 1.0f }, true, 0.0f, 1.0f, 0.0f,
            [pp] { return pp->IsVignetteEnabled() ? 1.0f : 0.0f; },
            [pp](float v) { if ((v > 0.5f) != pp->IsVignetteEnabled()) pp->ToggleVignette(); } });
        menuItems.push_back({ { 0.5f, 0.1f, 0.5f, 1.0f }, false, 0.0f, 1.0f, 0.01f,
            [pp] { return pp->GetVignetteIntensity(); }, [pp](float v) { pp->SetVignetteIntensity(v); } });
        menuItems.push_back({ { 0.5f, 0.1f, 0.5f, 1.0f }, false, 0.0f, 1.0f, 0.01f,
            [pp] { return pp->GetVignetteRadius(); }, [pp](float v) { pp->SetVignetteRadius(v); } });

        settingsMenu.Initialize(framework->GetSpriteCommon(), std::move(menuItems));
    }

    // 初期状態はプレイ中（マウス非表示・固定）
    isLookPaused = false;
    framework->GetWinApp()->ShowCursor(false);
    framework->GetWinApp()->SetClipCursor(true);
}

bool GameScene::CheckCollision(const Vector3& pos, float* outMaxY, float radius, Vector3* outPushDir) {
    float effectiveRadius = (radius > 0.0f) ? radius : collisionRadius;
    Vector3 center = { pos.x, pos.y + effectiveRadius, pos.z };
    std::vector<Object3d*> colliders;
    if (floor) colliders.push_back(floor.get());
    for (auto& w : walls) colliders.push_back(w.get());

    bool isHit = false;
    float highestY = -10000.0f; // 十分低い値
    float closestDistanceSq = FLT_MAX; // outPushDir用：最もめり込みが深い（＝距離が近い）障害物を探す
    Vector3 bestPushDir = { 0.0f, 0.0f, 0.0f };

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

        if (distanceSq <= effectiveRadius * effectiveRadius) {
            isHit = true;
            if (maxY > highestY) {
                highestY = maxY;
            }

            if (outPushDir && distanceSq < closestDistanceSq) {
                closestDistanceSq = distanceSq;
                if (distanceSq > 0.0001f) {
                    float dist = std::sqrt(distanceSq);
                    bestPushDir = { dx / dist, dy / dist, dz / dist };
                } else {
                    // 中心がちょうど面/辺/角の真上にある特異点。呼び出し側でゼロ扱いにする
                    bestPushDir = { 0.0f, 0.0f, 0.0f };
                }
            }
        }
    }

    if (outPushDir) *outPushDir = bestPushDir;

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

    // --- グレースケール表示の切り替え（Gキー） ---
    if (input->TriggerKey(DIK_G)) {
        postProcess->ToggleGrayscale();
    }

    // --- セピア表示の切り替え（Hキー。回想シーン風の演出用） ---
    if (input->TriggerKey(DIK_H)) {
        postProcess->ToggleSepia();
    }

    // --- 画面全体ぼかしの切り替え（Bキー） ---
    if (input->TriggerKey(DIK_B)) {
        postProcess->ToggleBlur();
    }

    // --- 画面周辺減光（ヴィネット）の切り替え（Vキー） ---
    if (input->TriggerKey(DIK_V)) {
        postProcess->ToggleVignette();
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

    // --- プレイヤーHPのテストダメージ（Nキー。ボス未実装のため動作確認用の仮トリガー） ---
    if (input->TriggerKey(DIK_N)) {
        TakeDamage(kTestDamageAmount);
    }

    // --- 武器モード切り替え（1:ライフル／2:ポータルガン） ---
    if (input->TriggerKey(DIK_1)) {
        weaponMode = WeaponMode::kRifle;
    }
    if (input->TriggerKey(DIK_2)) {
        weaponMode = WeaponMode::kPortalGun;
    }

    // --- ポータルガンの操作（左クリック:青ポータル／右クリック:オレンジポータル／R:リセット） ---
    if (!isLookPaused && weaponMode == WeaponMode::kPortalGun) {
        if (input->TriggerMouseButton(0) && !portalA.isPlaced) {
            PlacePortal(portalA);
        }
        if (input->TriggerMouseButton(1) && !portalB.isPlaced) {
            PlacePortal(portalB);
        }
        if (input->TriggerKey(DIK_R)) {
            portalA.isPlaced = false;
            portalB.isPlaced = false;
            if (portalA.visual) portalA.visual->SetScale({ 0.0f, 0.0f, 0.0f });
            if (portalB.visual) portalB.visual->SetScale({ 0.0f, 0.0f, 0.0f });
        }
    }

    // 移動方向の計算
    const float moveSpeed = 0.15f;
    moveDir = { 0, 0, 0 };
    Vector3 forward = { std::sin(cameraTransform.rotate.y), 0.0f, std::cos(cameraTransform.rotate.y) };
    Vector3 right = { std::cos(cameraTransform.rotate.y), 0.0f, -std::sin(cameraTransform.rotate.y) };

    if (input->PushKey(DIK_W)) { moveDir += forward; }
    if (input->PushKey(DIK_S)) { moveDir -= forward; }
    if (input->PushKey(DIK_A)) { moveDir -= right; }
    if (input->PushKey(DIK_D)) { moveDir += right; }

    if (Calculation::Length(moveDir) > 0) {
        moveDir = Calculation::Normalize(moveDir) * moveSpeed;
    }

    // テレポート直後の勢いを、減衰させながら移動方向へ加算する
    if (portalExitVelocityTimer > 0.0f) {
        moveDir += portalExitVelocity * (portalExitVelocityTimer / kPortalExitVelocityDuration);
        portalExitVelocityTimer -= 1.0f / 60.0f;
        if (portalExitVelocityTimer < 0.0f) portalExitVelocityTimer = 0.0f;
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
    // ポータルのテレポートに使う「勢いの継承」は、Y軸の衝突判定でvelocity.yが
    // 0にリセットされる前の値（重力・ジャンプ適用後の値）を使いたいので、ここで確保しておく
    float preCollisionVelocityY = velocity.y;
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

    // --- ポータルのテレポート判定 ---
    // X/Y/Z全ての移動・衝突判定が終わりplayerPosが確定した後に呼ぶ
    UpdatePortalTeleport(preCollisionVelocityY);

    // --- ヒットフラッシュの更新 ---
    for (auto it = hitFlashes.begin(); it != hitFlashes.end(); ) {
        it->timer -= 1.0f / 60.0f;
        if (it->timer <= 0) {
            // 時間切れなら元の色に戻す
            it->object->SetColor(it->originalColor);
            it = hitFlashes.erase(it);
        } else {
            ++it;
        }
    }

    // --- 敵ターゲットの復活タイマー更新 ---
    for (auto& enemy : enemies) {
        if (!enemy.isActive) {
            enemy.respawnTimer -= 1.0f / 60.0f;
            if (enemy.respawnTimer <= 0.0f) {
                enemy.isActive = true;
            }
        }
    }

    // --- ボスの移動AI（常に追いかける。距離が遠いほど速く、HPが減るほど怒り状態で速くなる） ---
    // playerPosがポータル移動も含めて確定した後（UpdatePortalTeleport済み）に計算する
    {
        Vector3 toPlayer = { playerPos.x - bossPos.x, 0.0f, playerPos.z - bossPos.z };
        float distance = Calculation::Length(toPlayer);

        if (distance > kBossStopDistance) {
            float distanceT = std::clamp((distance - kBossStopDistance) / (kBossFarDistance - kBossStopDistance), 0.0f, 1.0f);
            float baseSpeed = kBossSpeedNear + (kBossSpeedFar - kBossSpeedNear) * distanceT;

            float hpRatio = bossHP / kBossMaxHP;
            float enrageMultiplier = 1.0f + (kBossEnrageMaxMultiplier - 1.0f) * (1.0f - hpRatio);

            float bossMoveSpeed = baseSpeed * enrageMultiplier;
            Vector3 dir = Calculation::Normalize(toPlayer);
            Vector3 moveDelta = { dir.x * bossMoveSpeed, 0.0f, dir.z * bossMoveSpeed };

            const float kGroundOffset = 0.05f; // 床との判定を避けるためのオフセット（プレイヤー側と同じ手法）

            // まずは直進を試す
            Vector3 nextBossPos = bossPos;
            nextBossPos.x += moveDelta.x;
            nextBossPos.z += moveDelta.z;
            Vector3 checkBossPos = nextBossPos;
            checkBossPos.y += kGroundOffset;

            bool bossMoved = false;
            Vector3 pushDir{};
            if (!CheckCollision(checkBossPos, nullptr, kBossCollisionRadius, &pushDir)) {
                // 直進が通る＝障害物との接触が途切れたので、回避方向の保持を解除して直進に戻る
                bossAvoiding = false;
                bossPos = nextBossPos;
                bossMoved = true;
            } else {
                // 直進すると衝突する場合、実際に当たっている面の法線(pushDir)を使って壁沿いに滑らせる。
                // 直進方向から法線成分を取り除く（平面へ射影する）ことで、当たっている方向"以外"の
                // 成分だけが残り、具体的にどの障害物のどの面に当たっているかに応じて自然に回り込める
                // （左右をプレイヤーの位置関係だけで決め打ちする方式は、たまたま選んだ側が塞がっている
                // 袋小路で永久に詰んでしまう問題があったため、実際の衝突面を見る方式に変更した）
                // ただし衝突が続いている間、毎フレーム滑り方向を再計算すると角でガクつくため、
                // 塞がった最初のフレームだけ計算してbossAvoidDirに保持し、以後はそれを使い続ける。
                if (!bossAvoiding && (pushDir.x != 0.0f || pushDir.z != 0.0f)) {
                    Vector3 normal = { pushDir.x, 0.0f, pushDir.z };
                    float normalLen = Calculation::Length(normal);
                    if (normalLen > 0.0001f) {
                        normal = { normal.x / normalLen, 0.0f, normal.z / normalLen };
                        float dot = dir.x * normal.x + dir.z * normal.z;
                        Vector3 slideDir = { dir.x - dot * normal.x, 0.0f, dir.z - dot * normal.z };
                        float slideLen = Calculation::Length(slideDir);
                        if (slideLen > 0.0001f) {
                            slideDir = { slideDir.x / slideLen, 0.0f, slideDir.z / slideLen };
                            bossAvoidDir = slideDir;
                            bossAvoiding = true;
                        }
                    }
                }

                if (bossAvoiding) {
                    Vector3 slidePos = bossPos;
                    slidePos.x += bossAvoidDir.x * bossMoveSpeed;
                    slidePos.z += bossAvoidDir.z * bossMoveSpeed;
                    Vector3 checkSlidePos = slidePos;
                    checkSlidePos.y += kGroundOffset;
                    if (!CheckCollision(checkSlidePos, nullptr, kBossCollisionRadius)) {
                        bossPos = slidePos;
                        bossMoved = true;
                    } else {
                        // 保持していた回避方向が別の障害物に塞がれて進めなかった場合、
                        // そのまま保持し続けると恒久的に詰むため、保持を解除して次フレームで再計算させる
                        bossAvoiding = false;
                    }
                }
            }

            // 詰み検出＆後退フォールバック：直進もスライドも失敗する状態が一定時間続いたら、
            // プレイヤーから離れる方向へ後退を試みて角・隅の膠着から抜け出す（試行4の常時左右交互とは異なり、
            // あくまで最終手段としてのみ発動する）
            if (!bossMoved) {
                bossStuckTimer += 1.0f / 60.0f;
                if (bossStuckTimer > kBossStuckRetreatDelay) {
                    Vector3 retreatPos = bossPos;
                    retreatPos.x -= dir.x * bossMoveSpeed;
                    retreatPos.z -= dir.z * bossMoveSpeed;
                    Vector3 checkRetreatPos = retreatPos;
                    checkRetreatPos.y += kGroundOffset;
                    if (!CheckCollision(checkRetreatPos, nullptr, kBossCollisionRadius)) {
                        bossPos = retreatPos;
                        bossAvoiding = false;   // 後退後は仕切り直して直進から再判定させる
                        bossMoved = true;       // 動けたので詰み扱いを解除する
                        bossStuckTimer = 0.0f;  // このifブロックはbossMoved=false時にしか実行されないため明示的にリセットが必要
                    }
                }
            } else {
                bossStuckTimer = 0.0f;
            }

            // 移動方向へ向きを合わせる（プレイヤーのforwardベクトルと同じsin/cos規約）
            boss->SetRotate({ 0.0f, std::atan2(dir.x, dir.z), 0.0f });
        }

        boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });
    }

    cameraTransform.translate = { playerPos.x, playerPos.y + eyeHeight, playerPos.z };

    // --- カメラシェイクの更新・適用 ---
    // 残り時間の比率(1→0)に応じて強さを弱めながら、回転にランダムなオフセットを一時的に加える。
    // このオフセットはcamera->Update()とUpdateWeaponViewModel()（銃も一緒に揺れて見えるように）の
    // 両方に反映させたいので、それらの後でまとめて差し引いて元の視点に戻す。
    float shakeOffsetX = 0.0f;
    float shakeOffsetY = 0.0f;
    if (cameraShakeTimer > 0.0f) {
        cameraShakeTimer -= 1.0f / 60.0f;
        if (cameraShakeTimer < 0.0f) cameraShakeTimer = 0.0f;

        float shakeT = (cameraShakeDuration > 0.0f) ? (cameraShakeTimer / cameraShakeDuration) : 0.0f; // 1(直後)→0(平常)
        float currentStrength = cameraShakeStrength * shakeT;

        std::random_device seed_gen;
        std::mt19937 randomEngine(seed_gen());
        std::uniform_real_distribution<float> distShake(-currentStrength, currentStrength);
        shakeOffsetX = distShake(randomEngine);
        shakeOffsetY = distShake(randomEngine);

        cameraTransform.rotate.x += shakeOffsetX;
        cameraTransform.rotate.y += shakeOffsetY;
    }

    // カメラ行列を先に確定させる（視点武器などのObject3d::Update()が
    // このフレームのカメラ位置に対応したビュー行列を参照できるようにするため）
    camera->Update();

    // --- 射撃処理（ライフルモードの時のみ。ポータルガンモードの左右クリックは上で処理済み） ---
    if (!isLookPaused && weaponMode == WeaponMode::kRifle && input->TriggerMouseButton(0)) {
        FireShot();
    }

    // 視点武器の追従・リコイル更新（FireShot後に置き、当フレームからキックを反映する）
    UpdateWeaponViewModel();

    // カメラシェイクで加算したオフセットを差し引き、次フレームのマウス視点計算のベース値を汚さないようにする
    cameraTransform.rotate.x -= shakeOffsetX;
    cameraTransform.rotate.y -= shakeOffsetY;

    postProcess->Update(isLookPaused);
    settingsMenu.Update(input, isLookPaused);
    particleManagerLine->Update();
    particleManagerPoint->Update();
    particleManagerTriangle->Update();
    skybox->Update();
    floor->Update();
    for (auto& wall : walls) { wall->Update(); }
    for (auto& enemy : enemies) { enemy.object->Update(); }
    boss->Update();
    for (auto& decal : bulletDecals) { decal->Update(); }
    // 過去に敵ターゲットでUpdate()呼び忘れによりWVP行列が更新されず巨大に固定表示された
    // バグがあったため、ポータルの見た目も他のObject3dと同様に毎フレームUpdate()を呼ぶ
    if (portalA.visual) portalA.visual->Update();
    if (portalB.visual) portalB.visual->Update();
    reticle->Update();

    // HPバーの残量表示：現在HP比率に応じて塗りつぶし幅だけを縮める（背景は常にフル幅のまま）
    float hpRatio = playerHP / kPlayerMaxHP;
    hpBarFill->SetSize({ kHpBarMaxWidth * hpRatio, kHpBarHeight });
    hpBarBackground->Update();
    hpBarFill->Update();
    playerHpText.SetText(std::to_string(static_cast<int>(playerHP)));
    playerHpText.Update();

    // ボスHPバーの残量表示：現在HP比率に応じて塗りつぶし幅だけを縮める
    float bossHpRatio = bossHP / kBossMaxHP;
    bossHpBarFill->SetSize({ kBossHpBarMaxWidth * bossHpRatio, kBossHpBarHeight });
    bossHpBarBackground->Update();
    bossHpBarFill->Update();

#ifdef _DEBUG
    // "FPS Debug"パネルはDebugビルドの開発用（ライト/シャドウ調整等）。
    // Release/Developmentで調整したい項目はImGui非依存のsettingsMenuの方を使う
    if (isLookPaused) {
        ImGui::Begin("FPS Debug");
        if (ImGui::Button("RESUME GAME (P)")) {
            isLookPaused = !isLookPaused;
            winApp->ShowCursor(isLookPaused);
            winApp->SetClipCursor(!isLookPaused);
        }
        ImGui::SliderFloat("Sensitivity", &mouseSensitivity, 0.0001f, 0.01f, "%.4f");
        ImGui::SliderFloat("Eye Height", &eyeHeight, 0.1f, 3.0f, "%.1f");
        ImGui::DragFloat3("Weapon Body Offset", &weaponBodyLocalOffset.x, 0.01f);
        ImGui::DragFloat3("Weapon Barrel Offset", &weaponBarrelLocalOffset.x, 0.01f);
        ImGui::Text("Pos: %.2f, %.2f, %.2f", playerPos.x, playerPos.y, playerPos.z);

        if (ImGui::CollapsingHeader("Point Light")) {
            PointLight& pointLight = framework->GetObject3dCommon()->GetPointLightData();
            ImGui::ColorEdit3("PL Color", &pointLight.color.x);
            ImGui::DragFloat3("PL Position", &pointLight.position.x, 0.1f);
            ImGui::SliderFloat("PL Intensity", &pointLight.intensity, 0.0f, 10.0f);
            ImGui::SliderFloat("PL Radius", &pointLight.radius, 0.1f, 30.0f);
            ImGui::SliderFloat("PL Decay", &pointLight.decay, 0.1f, 5.0f);
        }
        if (ImGui::CollapsingHeader("Spot Light")) {
            SpotLight& spotLight = framework->GetObject3dCommon()->GetSpotLightData();
            ImGui::ColorEdit3("SL Color", &spotLight.color.x);
            ImGui::DragFloat3("SL Position", &spotLight.position.x, 0.1f);
            if (ImGui::DragFloat3("SL Direction", &spotLight.direction.x, 0.01f)) {
                spotLight.direction = Calculation::Normalize(spotLight.direction);
            }
            ImGui::SliderFloat("SL Intensity", &spotLight.intensity, 0.0f, 10.0f);
            ImGui::SliderFloat("SL Distance", &spotLight.distance, 0.1f, 30.0f);
            ImGui::SliderFloat("SL Decay", &spotLight.decay, 0.1f, 5.0f);
            ImGui::SliderFloat("SL Cos Angle", &spotLight.cosAngle, 0.0f, 1.0f);
            ImGui::SliderFloat("SL Cos Falloff Start", &spotLight.cosFalloffStart, 0.0f, 1.0f);
        }
        if (ImGui::CollapsingHeader("Shadow")) {
            float& shadowBias = framework->GetObject3dCommon()->GetShadowBias();
            ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0f, 0.02f, "%.4f");
        }

        ImGui::End();
    }
#endif
}

void GameScene::Draw() {
    Framework* framework = Framework::GetInstance();

    // --- 0. シャドウマップ生成パス（ディレクショナルライト視点の深度のみ描画） ---
    // 床・壁のみを影の落とし手として扱う（弾痕デカール・武器ビューモデル・パーティクルは対象外）
    Object3dCommon* object3dCommon = framework->GetObject3dCommon();
    object3dCommon->PreDrawShadow();
    Matrix4x4 lightViewProjection = object3dCommon->GetLightViewProjection();
    floor->DrawShadow(lightViewProjection);
    for (auto& wall : walls) { wall->DrawShadow(lightViewProjection); }
    for (auto& enemy : enemies) { if (enemy.isActive) enemy.object->DrawShadow(lightViewProjection); }
    if (boss) boss->DrawShadow(lightViewProjection);
    object3dCommon->PostDrawShadow();

    // --- 1. ポストプロセス用のオフスクリーンレンダリング開始 ---
    // ここで描画先をテクスチャに「寄り道」させる
    postProcess->PreDraw();

    // スカイボックスの描画
    framework->GetSkyboxCommon()->PreDraw();
    skybox->Draw();

    // 3Dオブジェクトの描画
    framework->GetObject3dCommon()->PreDraw();
    floor->Draw();
    for (auto& wall : walls) { wall->Draw(); }
    for (auto& enemy : enemies) { if (enemy.isActive) enemy.object->Draw(); }
    if (boss) boss->Draw();
    for (auto& decal : bulletDecals) { decal->Draw(); }
    // ワープポータル（薄い板なのでシャドウパスには含めない）
    if (portalA.isPlaced) portalA.visual->Draw();
    if (portalB.isPlaced) portalB.visual->Draw();

    // パーティクルの描画（点・線・面、それぞれ別のPSOで描画する）
    framework->GetParticleCommon()->PreDraw(particleManagerLine->GetDrawType());
    particleManagerLine->Draw();

    framework->GetParticleCommon()->PreDraw(particleManagerPoint->GetDrawType());
    particleManagerPoint->Draw();

    framework->GetParticleCommon()->PreDraw(particleManagerTriangle->GetDrawType());
    particleManagerTriangle->Draw();

    // --- 視点武器（常に最前面に描画。壁などにめり込んで消えないようにする） ---
    DrawWeaponViewModel();

    // 描画先を本来の画面（スワップチェーン）に「戻す」
    postProcess->PostDraw();
    // --- 1. オフスクリーンレンダリング終了 ---

    // --- 2. 画面への最終描画 ---
    // ポストプロセスの結果（テクスチャ）を画面いっぱいに描画
    // ここは専用の PSO/RootSignature で描画される
    postProcess->Draw();

    // --- 3. UI（レティクル）の描画 ---
    // PostProcess::Draw でルートシグネチャが書き換わっているため、
    // スプライトを描く前にもう一度 SpriteCommon::PreDraw を呼ぶ必要がある
    framework->GetSpriteCommon()->PreDraw();
    reticle->Draw();

    // --- 4. UI（プレイヤーHPバー）の描画 ---
    hpBarBackground->Draw();
    hpBarFill->Draw();
    playerHpText.Draw();

    // --- 5. UI（ボスHPバー）の描画 ---
    bossHpBarBackground->Draw();
    bossHpBarFill->Draw();

    // --- 6. UI（設定メニュー）の描画：Pキーでのポーズ中のみ表示 ---
    if (isLookPaused) {
        settingsMenu.Draw();
    }
}

void GameScene::Finalize() {
    // 終了時は必ずマウスを解放する
    Framework::GetInstance()->GetWinApp()->ShowCursor(true);
    Framework::GetInstance()->GetWinApp()->SetClipCursor(false);
}

void GameScene::TriggerCameraShake(float duration, float strength) {
    cameraShakeTimer = duration;
    cameraShakeDuration = duration;
    cameraShakeStrength = strength;
}

void GameScene::TakeDamage(float amount) {
    playerHP -= amount;
    if (playerHP < 0.0f) {
        playerHP = 0.0f;
    }
    TriggerCameraShake(kPlayerDamageShakeDuration, kPlayerDamageShakeStrength);
    postProcess->TriggerDamageVignette();
}

void GameScene::DamageBoss(float amount) {
    bossHP -= amount;
    if (bossHP < 0.0f) {
        bossHP = 0.0f;
    }
}

void GameScene::FireShot() {
    // 命中判定に関わらず、発砲した時点でリコイル演出とカメラシェイクを開始する
    weaponRecoilTimer = kRecoilDuration;
    TriggerCameraShake(kFireShakeDuration, kFireShakeStrength);

    Transform camTrans = camera->GetTransform();
    
    // カメラの回転から前方ベクトルを計算
    // rotate.x が上下、rotate.y が左右の回転
    Vector3 direction;
    direction.x = std::cos(camTrans.rotate.x) * std::sin(camTrans.rotate.y);
    direction.y = std::sin(-camTrans.rotate.x);
    direction.z = std::cos(camTrans.rotate.x) * std::cos(camTrans.rotate.y);
    direction = Calculation::Normalize(direction);

    // ==================== マズルフラッシュ（命中の有無に関わらず、発砲した瞬間に必ず出す） ====================
    // 派手め（DOOM Eternal/Valorant系）の演出にするため、パッと明るく光ってすぐ消えるフラッシュ＋勢いよく飛ぶ火花にする。
    // 銃口位置はUpdateWeaponViewModel()と同じ計算式（カメラのワールド行列 + weaponBarrelLocalOffset）を
    // ここで自己完結して再計算し、フレーム順序に依存しないようにする。
    {
        std::random_device seed_gen;
        std::mt19937 randomEngine(seed_gen());

        Matrix4x4 cameraWorldMatrix = Calculation::MakeAffineMatrix(
            { 1.0f, 1.0f, 1.0f }, camTrans.rotate, camTrans.translate);
        Vector3 muzzleLocal = weaponBarrelLocalOffset;
        muzzleLocal.z += 0.16f; // 銃身(half-extentが0.16f)の先端＝銃口位置
        Vector3 muzzleWorld = Calculation::Transform(muzzleLocal, cameraWorldMatrix);

        // 壁にめり込んで見えないよう、前方へ少しだけ出す
        Vector3 muzzlePos = {
            muzzleWorld.x + direction.x * 0.05f,
            muzzleWorld.y + direction.y * 0.05f,
            muzzleWorld.z + direction.z * 0.05f,
        };

        // --- 面（閃光本体）：細長い長方形を3枚、60度おきに重ねて六芒星(星型)の閃光にする ---
        {
            std::uniform_real_distribution<float> distLife(0.05f, 0.07f);
            std::uniform_real_distribution<float> distSize(0.1f, 0.13f);
            std::uniform_real_distribution<float> distBaseRoll(0.0f, 6.28318f); // 毎回向きをランダムにして単調さを消す

            float size = distSize(randomEngine);
            float lifeTime = distLife(randomEngine);
            float baseRoll = distBaseRoll(randomEngine);
            constexpr float kSixthTurn = 1.04719755f; // 60度

            // 細長い長方形(x=長さ方向, y=太さ方向)を作って発生させるヘルパー
            auto EmitFlashBar = [&](float roll) {
                ParticleEmitParams params;
                params.position = muzzlePos;
                params.velocity = { 0.0f, 0.0f, 0.0f };
                params.lifeTime = lifeTime;

                // 明るい白〜黄色からオレンジ寄りの透明へ、一瞬で消える
                params.useColorGradient = true;
                params.startColor = { 1.0f, 0.9f, 0.6f, 0.85f };
                params.endColor = { 1.0f, 0.6f, 0.2f, 0.0f };

                // サイズは縮めず、色のフェードだけで消す(縮小させると光ではなく萎むように見えるため)
                params.startScale = { size * 2.2f, size * 0.3f, size };
                params.endScale = params.startScale;

                params.roll = roll;

                particleManagerTriangle->Emit(params);
            };

            // 60度ずつずらして3枚重ねる。1枚のバーが左右2方向に伸びるので、先端が6つの星型になる
            EmitFlashBar(baseRoll);
            EmitFlashBar(baseRoll + kSixthTurn);
            EmitFlashBar(baseRoll + kSixthTurn * 2.0f);

            // 中心から外側へ：芯(小・高輝度) → ミドル(中・中間色) → グロー(大・高透明度)の3層で
            // 疑似的な放射グラデーション(中心が明るく外側ほど透明)を作る
            ParticleEmitParams core;
            core.position = muzzlePos;
            core.velocity = { 0.0f, 0.0f, 0.0f };
            core.lifeTime = lifeTime;
            core.useColorGradient = true;
            core.startColor = { 1.0f, 0.95f, 0.8f, 0.95f };
            core.endColor = { 1.0f, 0.6f, 0.2f, 0.0f };
            core.startScale = { size * 0.4f, size * 0.4f, size * 0.4f };
            core.endScale = core.startScale; // サイズ固定、色のフェードだけで消す
            particleManagerTriangle->Emit(core);

            ParticleEmitParams midParams = core;
            midParams.startColor = { 1.0f, 0.9f, 0.6f, 0.55f };
            midParams.startScale = { size * 1.1f, size * 1.1f, size * 1.1f };
            midParams.endScale = midParams.startScale;
            midParams.lifeTime = lifeTime * 1.15f;
            particleManagerTriangle->Emit(midParams);

            ParticleEmitParams glowParams = core;
            glowParams.startColor = { 1.0f, 0.85f, 0.5f, 0.25f };
            glowParams.startScale = { size * 2.2f, size * 2.2f, size * 2.2f };
            glowParams.endScale = glowParams.startScale;
            glowParams.lifeTime = lifeTime * 1.3f;
            particleManagerTriangle->Emit(glowParams);
        }

        // --- 点（銃口から前方へ勢いよく飛ぶ火花） ---
        {
            std::uniform_int_distribution<int> distCount(5, 8);
            std::uniform_real_distribution<float> distSpeed(6.0f, 10.0f); // 既存の着弾火花より速めに飛ばす
            std::uniform_real_distribution<float> distLife(0.08f, 0.15f);
            std::uniform_real_distribution<float> distJitter(-0.15f, 0.15f); // 前方向を中心に少しだけ散らす
            int count = distCount(randomEngine);

            for (int i = 0; i < count; ++i) {
                Vector3 dir = Calculation::Normalize(Vector3{
                    direction.x + distJitter(randomEngine),
                    direction.y + distJitter(randomEngine),
                    direction.z + distJitter(randomEngine),
                    });
                float speed = distSpeed(randomEngine);

                ParticleEmitParams params;
                params.position = muzzlePos;
                params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
                params.lifeTime = distLife(randomEngine);

                params.useColorGradient = true;
                params.startColor = { 1.0f, 0.9f, 0.5f, 1.0f };
                params.endColor = { 1.0f, 0.3f, 0.0f, 0.0f };

                params.gravity = { 0.0f, -0.004f, 0.0f };
                params.drag = 0.15f;

                params.startScale = { 1.0f, 1.0f, 1.0f };
                params.endScale = { 0.2f, 0.2f, 0.2f };

                particleManagerPoint->Emit(params);
            }
        }
    }

    Ray ray = { camTrans.translate, direction };
    
    // 全ての壁と床を判定対象にする
    std::vector<Object3d*> targets;
    if (floor) targets.push_back(floor.get());
    for (auto& w : walls) targets.push_back(w.get());
    for (auto& enemy : enemies) {
        if (enemy.isActive) targets.push_back(enemy.object.get());
    }
    if (boss) targets.push_back(boss.get());

    RaycastHit closestHit;
    AABB closestAABB{};
    Object3d* closestObject = nullptr;
    RaycastClosest(ray, targets, &closestHit, &closestAABB, &closestObject);

    if (closestObject) {
        // ヒットした場合の演出
        // 1. パーティクル（火花）を出す：線（飛び散る火花）・点（細かい火の粉）・面（着弾フラッシュ）を同時に生成する
        std::random_device seed_gen;
        std::mt19937 randomEngine(seed_gen());
        // 火花の中心方向を決める。
        // 「面の法線」だけだと地面で真上への噴水になってしまうので、
        // 「撃った方向の逆向き（＝手前・撃った側へ戻る向き）」をブレンドして跳ね返り感を出す。
        Vector3 normal = closestHit.normal;
        if (normal.x == 0.0f && normal.y == 0.0f && normal.z == 0.0f) {
            // 法線が取れていない場合は入射方向の逆向きで代用
            normal = { -direction.x, -direction.y, -direction.z };
        }
        normal = Calculation::Normalize(normal);

        Vector3 reverse = { -direction.x, -direction.y, -direction.z }; // 撃った方向の逆
        const float kSparkBackBias = 1.0f; // 手前へ寄せる強さ（0=法線のみ／大きいほど撃った側へ強く戻る）
        Vector3 sparkDir = {
            normal.x + reverse.x * kSparkBackBias,
            normal.y + reverse.y * kSparkBackBias,
            normal.z + reverse.z * kSparkBackBias,
        };
        sparkDir = Calculation::Normalize(sparkDir);
        std::uniform_real_distribution<float> distAngle(0.0f, 6.28318f);

        // 指定した中心方向まわりに、coneAngle の範囲でランダムにばらまいた単位ベクトルを返す。
        // 中心に垂直な基底(tangent/bitangent)を Cross で作り、その平面上へ散らす。
        auto coneDir = [&](const Vector3& center, float coneAngle) {
            Vector3 arbitrary = (std::abs(center.y) < 0.99f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
            Vector3 tangent = Calculation::Normalize(Calculation::Cross(arbitrary, center));
            Vector3 bitangent = Calculation::Cross(center, tangent);

            std::uniform_real_distribution<float> distConeT(std::cos(coneAngle), 1.0f);
            float cosAngle = distConeT(randomEngine);
            float sinAngle = std::sqrt((std::max)(0.0f, 1.0f - cosAngle * cosAngle));
            float phi = distAngle(randomEngine);
            return Vector3{
                tangent.x * sinAngle * std::cos(phi) + bitangent.x * sinAngle * std::sin(phi) + center.x * cosAngle,
                tangent.y * sinAngle * std::cos(phi) + bitangent.y * sinAngle * std::sin(phi) + center.y * cosAngle,
                tangent.z * sinAngle * std::cos(phi) + bitangent.z * sinAngle * std::sin(phi) + center.z * cosAngle,
            };
        };

        // --- 線（飛び散る火花の本体） ---
        {
            std::uniform_real_distribution<float> distSpeed(2.0f, 6.0f);
            std::uniform_real_distribution<float> distLife(0.25f, 0.5f);

            for (int i = 0; i < 30; ++i) {
                Vector3 dir = coneDir(sparkDir, 0.6f);
                float speed = distSpeed(randomEngine);

                ParticleEmitParams params;
                params.position = closestHit.hitPoint;
                params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
                params.lifeTime = distLife(randomEngine);

                // 見た目の時間変化：明るい黄色→赤くフェードしながら消える
                params.useColorGradient = true;
                params.startColor = { 1.0f, 0.9f, 0.4f, 1.0f };
                params.endColor = { 1.0f, 0.1f, 0.0f, 0.0f };

                // 物理：重力で落ち、空気抵抗で減速する
                params.gravity = { 0.0f, -0.015f, 0.0f };
                params.drag = 0.08f;

                // 線の太さと長さ（速度から毎フレーム自動計算される）。
                // 速くても集中線のようにならないよう、長さに上限をかける
                params.lineWidth = 0.04f;
                params.lineLengthMultiplier = 4.0f;
                params.maxLineLength = 0.4f;

                particleManagerLine->Emit(params);
            }
        }

        // --- 点（細かい火の粉、ふわっと漂うイメージ） ---
        {
            std::uniform_real_distribution<float> distSpeed(0.3f, 1.5f);
            std::uniform_real_distribution<float> distLife(0.4f, 0.9f);

            for (int i = 0; i < 25; ++i) {
                Vector3 dir = coneDir(sparkDir, 1.2f); // 線より広めに散らす
                float speed = distSpeed(randomEngine);

                ParticleEmitParams params;
                params.position = closestHit.hitPoint;
                params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
                params.lifeTime = distLife(randomEngine);

                params.useColorGradient = true;
                params.startColor = { 1.0f, 0.8f, 0.3f, 1.0f };
                params.endColor = { 1.0f, 0.2f, 0.0f, 0.0f };

                // 点は軽いのでゆっくり落ちる・空気抵抗強め
                params.gravity = { 0.0f, -0.004f, 0.0f };
                params.drag = 0.15f;

                params.startScale = { 1.0f, 1.0f, 1.0f };
                params.endScale = { 0.2f, 0.2f, 0.2f };

                particleManagerPoint->Emit(params);
            }
        }

        // --- 面（大小さまざまな飛び散る火花。ビルボード板ポリなのでサイズの違いがちゃんと見える） ---
        {
            std::uniform_real_distribution<float> distSpeed(1.0f, 4.0f);
            std::uniform_real_distribution<float> distLife(0.3f, 0.7f);
            std::uniform_real_distribution<float> distSize(0.03f, 0.18f); // 小さい粒〜大きめの粒までばらつかせる
            std::uniform_real_distribution<float> distRoll(0.0f, 6.28318f);  // 面内回転をランダムに
            std::uniform_real_distribution<float> distShear(-0.6f, 0.6f);    // 平行四辺形のような歪みをランダムに

            for (int i = 0; i < 20; ++i) {
                Vector3 dir = coneDir(sparkDir, 0.9f);
                float speed = distSpeed(randomEngine);
                float size = distSize(randomEngine);

                ParticleEmitParams params;
                params.position = closestHit.hitPoint;
                params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
                params.lifeTime = distLife(randomEngine);

                params.useColorGradient = true;
                params.startColor = { 1.0f, 0.85f, 0.3f, 1.0f };
                params.endColor = { 1.0f, 0.15f, 0.0f, 0.0f };

                params.gravity = { 0.0f, -0.012f, 0.0f };
                params.drag = 0.1f;

                // 発生時のサイズをランダムにし、消える間際は少し小さくして自然にフェードさせる
                params.startScale = { size, size, size };
                params.endScale = { size * 0.3f, size * 0.3f, size * 0.3f };

                // 見た目に角度のばらつきを出す：面内回転＋平行四辺形のような歪み
                params.roll = distRoll(randomEngine);
                params.shear = distShear(randomEngine);

                particleManagerTriangle->Emit(params);
            }
        }

        // --- 粉塵（インパクトパフ）：法線中心の広角コーンに、灰色の煙が膨らみながらゆっくり漂う ---
        {
            std::uniform_int_distribution<int> distCount(6, 8);
            std::uniform_real_distribution<float> distSpeed(0.2f, 0.6f);
            std::uniform_real_distribution<float> distLife(0.5f, 0.9f);
            int count = distCount(randomEngine);

            for (int i = 0; i < count; ++i) {
                Vector3 dir = coneDir(normal, 1.4f); // 火花よりさらに広角に、面の法線を中心に散らす
                float speed = distSpeed(randomEngine);

                ParticleEmitParams params;
                params.position = closestHit.hitPoint;
                params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
                params.lifeTime = distLife(randomEngine);

                // 灰色から透明へフェード
                params.useColorGradient = true;
                params.startColor = { 0.5f, 0.5f, 0.5f, 0.5f };
                params.endColor = { 0.5f, 0.5f, 0.5f, 0.0f };

                // 煙なのでほぼ無重力、空気抵抗は強めにしてすぐ漂うように減速させる
                params.gravity = { 0.0f, 0.0f, 0.0f };
                params.drag = 0.3f;

                // 膨らみながら消えるパフ効果
                params.startScale = { 0.15f, 0.15f, 0.15f };
                params.endScale = { 0.5f, 0.5f, 0.5f };

                particleManagerTriangle->Emit(params);
            }
        }

        // 敵ターゲット（的）へのヒットかどうかを判定する
        auto enemyIt = std::find_if(enemies.begin(), enemies.end(), [&](const EnemyTarget& e) {
            return e.object.get() == closestObject;
        });

        if (enemyIt != enemies.end()) {
            // 的にヒットした場合：弾痕デカール・赤フラッシュは行わず、非アクティブ化して復活待ちにする
            enemyIt->isActive = false;
            enemyIt->respawnTimer = kEnemyRespawnDelay;

            // カメラシェイク：的でも命中の「重み」を出す
            TriggerCameraShake(kHitShakeDuration, kHitShakeStrength);

            Logger::Log("Hit! Distance: " + std::to_string(closestHit.distance) + "\n");
        } else {
            if (closestObject == boss.get()) {
                DamageBoss(kBossDamagePerShot);
            }

            SpawnBulletDecal(closestHit.hitPoint, normal, closestAABB);

            // 2. 壁の色を一時的に変える
            auto it = std::find_if(hitFlashes.begin(), hitFlashes.end(), [&](const HitFlash& f) {
                return f.object == closestObject;
            });

            if (it != hitFlashes.end()) {
                // 既にフラッシュ中ならタイマーをリセット
                it->timer = 0.2f;
            } else {
                // 新しくフラッシュを開始
                HitFlash flash;
                flash.object = closestObject;
                flash.timer = 0.2f;
                flash.originalColor = closestObject->GetColor();
                hitFlashes.push_back(flash);
            }

            closestObject->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤くする

            // 3. カメラシェイク：発砲時より少し大きく揺らして命中の「重み」を出す
            TriggerCameraShake(kHitShakeDuration, kHitShakeStrength);

            Logger::Log("Hit! Distance: " + std::to_string(closestHit.distance) + "\n");
        }
    } else {
        Logger::Log("Miss...\n");
    }
}

void GameScene::SpawnBulletDecal(const Vector3& position, const Vector3& normal, const AABB& hitAABB) {
    Object3d* decal = bulletDecals[nextDecalIndex].get();
    nextDecalIndex = (nextDecalIndex + 1) % static_cast<int>(bulletDecals.size());

    // plane.objの平面はXY平面・法線+Zを向いているので、+Zから命中面の法線への回転を作る
    Quaternion rotation = Calculation::DirectionToDirection({ 0.0f, 0.0f, 1.0f }, normal);
    decal->SetRotateQuaternion(rotation);

    // 角付近に着弾すると、面に貼り付けた弾痕(平面)が箱の外側にはみ出して浮いて見えてしまう。
    // → 位置はずらさず、はみ出す分だけ弾痕のスケールを縮小して面の内側に収める。
    const float kDecalHalfSize = 0.15f; // 弾痕プレートの基本半サイズ(SetScaleの基準値)

    // 端からの距離(はみ出さずに置ける半サイズ)に応じた縮小率を返す
    auto EdgeScale = [](float pos, float minV, float maxV, float halfSize) {
        float availableHalf = (std::min)(pos - minV, maxV - pos);
        return std::clamp(availableHalf / halfSize, 0.25f, 1.0f);
    };

    // decalの平面はZ+基準で法線方向へ回転しているため、ローカルX/Y軸が
    // 世界のどの軸に対応するかは法線の向きで変わる(DirectionToDirectionは
    // 軸整列な90度回転にしかならないため、対応パターンは以下の3通りのみ)。
    float scaleLocalX;
    float scaleLocalY;
    if (std::abs(normal.x) > 0.5f) {
        // 法線=X → ローカルX=ワールドZ, ローカルY=ワールドY
        scaleLocalX = EdgeScale(position.z, hitAABB.min.z, hitAABB.max.z, kDecalHalfSize);
        scaleLocalY = EdgeScale(position.y, hitAABB.min.y, hitAABB.max.y, kDecalHalfSize);
    } else if (std::abs(normal.y) > 0.5f) {
        // 法線=Y → ローカルX=ワールドX, ローカルY=ワールドZ
        scaleLocalX = EdgeScale(position.x, hitAABB.min.x, hitAABB.max.x, kDecalHalfSize);
        scaleLocalY = EdgeScale(position.z, hitAABB.min.z, hitAABB.max.z, kDecalHalfSize);
    } else {
        // 法線=Z → ローカルX=ワールドX, ローカルY=ワールドY
        scaleLocalX = EdgeScale(position.x, hitAABB.min.x, hitAABB.max.x, kDecalHalfSize);
        scaleLocalY = EdgeScale(position.y, hitAABB.min.y, hitAABB.max.y, kDecalHalfSize);
    }
    decal->SetScale({ kDecalHalfSize * scaleLocalX, kDecalHalfSize * scaleLocalY, kDecalHalfSize });

    // Zファイティング回避のため法線方向に少しオフセット(位置は実際の命中点のまま)
    decal->SetTranslate({
        position.x + normal.x * 0.02f,
        position.y + normal.y * 0.02f,
        position.z + normal.z * 0.02f
        });
}

void GameScene::UpdateWeaponViewModel() {
    Transform& cameraTransform = camera->GetTransform();

    // リコイルタイマーの減衰（HitFlashと同じ固定タイムステップ 1/60秒）
    if (weaponRecoilTimer > 0.0f) {
        weaponRecoilTimer -= 1.0f / 60.0f;
        if (weaponRecoilTimer < 0.0f) weaponRecoilTimer = 0.0f;
    }
    float recoilT = weaponRecoilTimer / kRecoilDuration; // 1(直後)→0(平常)
    float recoilKickZ = -kRecoilKickAmount * recoilT;    // カメラ手前方向へ引く

    // カメラのワールド行列（Camera::Update()が使っているものと同一の計算式）
    Matrix4x4 cameraWorldMatrix = Calculation::MakeAffineMatrix(
        { 1.0f, 1.0f, 1.0f }, cameraTransform.rotate, cameraTransform.translate);

    Vector3 bodyLocal = weaponBodyLocalOffset;
    bodyLocal.z += recoilKickZ;
    Vector3 barrelLocal = weaponBarrelLocalOffset;
    barrelLocal.z += recoilKickZ;

    Vector3 bodyWorld = Calculation::Transform(bodyLocal, cameraWorldMatrix);
    Vector3 barrelWorld = Calculation::Transform(barrelLocal, cameraWorldMatrix);

    weaponBody->SetTranslate(bodyWorld);
    weaponBody->SetRotate(cameraTransform.rotate);

    weaponBarrel->SetTranslate(barrelWorld);
    weaponBarrel->SetRotate(cameraTransform.rotate);

    weaponBody->Update();
    weaponBarrel->Update();
}

void GameScene::DrawWeaponViewModel() {
    Framework* framework = Framework::GetInstance();
    auto commandList = framework->GetDxCommon()->GetCommandList();

    // 直前にParticleCommonのPSOがバインドされているため、Object3d用に再バインドする
    framework->GetObject3dCommon()->PreDraw();

    // 深度バッファのみをクリアし、既に描画済みの壁などにめり込んで隠れるのを防ぐ
    commandList->ClearDepthStencilView(
        framework->GetDxCommon()->GetDsvHandle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    weaponBody->Draw();
    weaponBarrel->Draw();
}

AABB GameScene::GetAABB(const Object3d& object) {
    Transform t = object.GetTransform();
    AABB aabb;
    aabb.min = { t.translate.x - t.scale.x, t.translate.y - t.scale.y, t.translate.z - t.scale.z };
    aabb.max = { t.translate.x + t.scale.x, t.translate.y + t.scale.y, t.translate.z + t.scale.z };
    return aabb;
}

bool GameScene::RaycastClosest(const Ray& ray, const std::vector<Object3d*>& targets, RaycastHit* outHit, AABB* outAABB, Object3d** outObject) {
    Object3d* closestObject = nullptr;
    float minDistance = FLT_MAX;
    RaycastHit hit;
    RaycastHit closestHit;
    AABB closestAABB{};

    for (Object3d* obj : targets) {
        AABB aabb = GetAABB(*obj);
        if (Calculation::TestRayAABB(ray, aabb, &hit)) {
            if (hit.distance < minDistance) {
                minDistance = hit.distance;
                closestObject = obj;
                closestHit = hit;
                closestAABB = aabb;
            }
        }
    }

    if (!closestObject) {
        return false;
    }

    if (outHit) *outHit = closestHit;
    if (outAABB) *outAABB = closestAABB;
    if (outObject) *outObject = closestObject;
    return true;
}

void GameScene::PlacePortal(Portal& portal) {
    Transform camTrans = camera->GetTransform();

    // カメラの回転から前方ベクトルを計算（FireShot()と同じ計算式）
    Vector3 direction;
    direction.x = std::cos(camTrans.rotate.x) * std::sin(camTrans.rotate.y);
    direction.y = std::sin(-camTrans.rotate.x);
    direction.z = std::cos(camTrans.rotate.x) * std::cos(camTrans.rotate.y);
    direction = Calculation::Normalize(direction);

    Ray ray = { camTrans.translate, direction };

    // ポータルは床・壁にのみ設置できる（的は対象外）。
    // ボスは設置面にはしないが、視線を遮る遮蔽物としてはレイキャスト対象に含める必要がある
    // （含めないと、ボスに照準を合わせた時にボスの奥の壁まで貫通判定されてしまい、
    //   ボスの陰に隠れて見えない位置にポータルが設置＝isPlacedだけtrueになる不具合が起きる）
    std::vector<Object3d*> targets;
    if (floor) targets.push_back(floor.get());
    for (auto& w : walls) targets.push_back(w.get());
    if (boss) targets.push_back(boss.get());

    RaycastHit hit;
    AABB hitAABB{};
    Object3d* hitObject = nullptr;
    if (!RaycastClosest(ray, targets, &hit, &hitAABB, &hitObject)) {
        return; // 何もヒットしなければ設置しない
    }
    if (hitObject == boss.get()) {
        return; // ボスは遮蔽物としてレイを止めるだけで、設置面にはしない
    }

    Vector3 normal = hit.normal;
    if (normal.x == 0.0f && normal.y == 0.0f && normal.z == 0.0f) {
        // 法線が取れていない場合は入射方向の逆向きで代用（FireShot()と同じフォールバック）
        normal = { -direction.x, -direction.y, -direction.z };
    }
    normal = Calculation::Normalize(normal);

    // Zファイティング回避のため法線方向に少しオフセット（SpawnBulletDecalと同じ手法）
    portal.position = {
        hit.hitPoint.x + normal.x * 0.02f,
        hit.hitPoint.y + normal.y * 0.02f,
        hit.hitPoint.z + normal.z * 0.02f,
    };
    portal.normal = normal;
    portal.orientation = Calculation::DirectionToDirection({ 0.0f, 0.0f, 1.0f }, normal);
    portal.isPlaced = true;

    if (!portal.visual) {
        Framework* framework = Framework::GetInstance();
        portal.visual = std::make_unique<Object3d>();
        portal.visual->Initialize(framework->GetObject3dCommon());
        portal.visual->SetModel(planeModel.get());
    }

    portal.visual->SetRotateQuaternion(portal.orientation);

    // SpawnBulletDecalと同じ要領で、当たった面(AABB)の端からはみ出す分だけスケールを縮小する。
    // デカール(0.15)よりずっと大きく、プレイヤーが通れるサイズ感にする。
    const float kPortalHalfSize = 1.5f;

    auto EdgeScale = [](float pos, float minV, float maxV, float halfSize) {
        float availableHalf = (std::min)(pos - minV, maxV - pos);
        return std::clamp(availableHalf / halfSize, 0.25f, 1.0f);
    };

    float scaleLocalX;
    float scaleLocalY;
    if (std::abs(normal.x) > 0.5f) {
        scaleLocalX = EdgeScale(portal.position.z, hitAABB.min.z, hitAABB.max.z, kPortalHalfSize);
        scaleLocalY = EdgeScale(portal.position.y, hitAABB.min.y, hitAABB.max.y, kPortalHalfSize);
    } else if (std::abs(normal.y) > 0.5f) {
        scaleLocalX = EdgeScale(portal.position.x, hitAABB.min.x, hitAABB.max.x, kPortalHalfSize);
        scaleLocalY = EdgeScale(portal.position.z, hitAABB.min.z, hitAABB.max.z, kPortalHalfSize);
    } else {
        scaleLocalX = EdgeScale(portal.position.x, hitAABB.min.x, hitAABB.max.x, kPortalHalfSize);
        scaleLocalY = EdgeScale(portal.position.y, hitAABB.min.y, hitAABB.max.y, kPortalHalfSize);
    }
    portal.visual->SetScale({ kPortalHalfSize * scaleLocalX, kPortalHalfSize * scaleLocalY, kPortalHalfSize });
    portal.visual->SetTranslate(portal.position);

    // portalAは青、portalBはオレンジで区別する
    Vector4 color = (&portal == &portalA)
        ? Vector4{ 0.2f, 0.5f, 1.0f, 1.0f }
        : Vector4{ 1.0f, 0.55f, 0.1f, 1.0f };
    portal.visual->SetColor(color);
}

Vector3 GameScene::TransformThroughPortal(const Portal& from, const Portal& to, const Vector3& v) {
    // 1. fromの姿勢を打ち消して正規化空間（法線+Z基準）に戻す
    Quaternion undoFrom = Calculation::Conjugate(from.orientation);
    // 2. 正規化空間内でY軸まわりに180度回転させる（ポータルの表裏を反転させ、
    //    入ってきた勢いを出口の向こう側へ抜けるように反転させる）
    Quaternion flip180 = Calculation::MakeAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, 3.14159265358979323846f);
    // 3. toの姿勢でワールド空間に戻す
    // Calculation::Multiply(a, b) は「bを先に適用してからaを適用する」合成になる
    // （Object3d::SetRotate()のオイラー→クォータニオン合成と同じ規則）ため、
    // 適用したい順（undoFrom→flip180→to.orientation）と逆の並びで呼び出す。
    Quaternion combined = Calculation::Multiply(to.orientation, Calculation::Multiply(flip180, undoFrom));

    Matrix4x4 rotateMatrix = Calculation::MakeRotateMatrix(combined);
    return Calculation::Transform(v, rotateMatrix);
}

void GameScene::UpdatePortalTeleport(float preCollisionVelocityY) {
    if (portalCooldownTimer > 0.0f) {
        portalCooldownTimer -= 1.0f / 60.0f;
        return;
    }

    if (!(portalA.isPlaced && portalB.isPlaced)) {
        return;
    }

    const Portal* entry = nullptr;
    const Portal* exit = nullptr;
    // playerPosは足元の座標のため、目線の高さ付近など体の途中に設置されたポータルとの距離を
    // 単純に測ると立っているだけでは届かない（ジャンプで足元が浮いた時だけ偶然届いてしまう）。
    // 足元(playerPos.y)〜頭(playerPos.y+eyeHeight)の区間上でポータルに最も近い点を求めて判定する。
    auto ClosestPointOnBody = [this](const Vector3& portalPos) {
        float clampedY = std::clamp(portalPos.y, playerPos.y, playerPos.y + eyeHeight);
        return Vector3{ playerPos.x, clampedY, playerPos.z };
    };
    if (Calculation::Length(ClosestPointOnBody(portalA.position) - portalA.position) < kPortalRadius) {
        entry = &portalA;
        exit = &portalB;
    } else if (Calculation::Length(ClosestPointOnBody(portalB.position) - portalB.position) < kPortalRadius) {
        entry = &portalB;
        exit = &portalA;
    }

    if (!entry) {
        return;
    }

    Vector3 intent = { moveDir.x, preCollisionVelocityY, moveDir.z };
    Vector3 exitVector = TransformThroughPortal(*entry, *exit, intent);

    playerPos = {
        exit->position.x + exit->normal.x * (collisionRadius + 0.1f),
        exit->position.y + exit->normal.y * (collisionRadius + 0.1f),
        exit->position.z + exit->normal.z * (collisionRadius + 0.1f),
    };
    velocity.y = exitVector.y;
    portalExitVelocity = { exitVector.x, 0.0f, exitVector.z };
    portalExitVelocityTimer = kPortalExitVelocityDuration;
    portalCooldownTimer = kPortalTeleportCooldown;

    // ワープ通過の瞬間、Apex/Titanfallのフェイズ移動のような一瞬のグレースケール演出を入れる
    postProcess->TriggerGrayscaleFlash();
}
