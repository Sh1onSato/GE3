#include "GameScene.h"
#include "Framework.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "CameraManager.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "GameSettings.h"
#include "Logger.h"
#include "MyGame.h"
#include "Easing.h"
#include "externals/imgui/imgui.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <random>

void GameScene::Initialize() {
    Framework* framework = Framework::GetInstance();

    // タイトル画面で調整された値を引き継ぐ
    mouseSensitivity = GameSettings::GetInstance()->GetMouseSensitivity();
    eyeHeight = GameSettings::GetInstance()->GetEyeHeight();

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

    screenEffect = MyGame::GetInstance()->GetScreenEffect();
    screenEffect->ResetTransientEffects();

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

    // --- 武器表示（画面右下）。配置は毎フレーム UpdateWeaponHud で今の武器に合わせて決める ---
    {
        const std::array<const char*, kWeaponIconCount> iconFiles = {
            "Resources/ui/weapon_shotgun.png", "Resources/ui/weapon_shotgun_gray.png",
            "Resources/ui/weapon_rifle.png", "Resources/ui/weapon_rifle_gray.png",
        };
        for (int i = 0; i < kWeaponIconCount; ++i) {
            TextureManager::GetInstance()->LoadTexture(iconFiles[i]);
            weaponIcons[i] = std::make_unique<Sprite>();
            weaponIcons[i]->Initialize(framework->GetSpriteCommon(), iconFiles[i]);
            weaponIcons[i]->SetAnchorPoint({ 1.0f, 1.0f });
        }
        // 武器の後ろのパネル（位置・大きさは固定なのでここで決める）
        const std::array<Vector2, 2> panelPos = { kWeaponPanelBackPos, kWeaponPanelFrontPos };
        const std::array<Vector2, 2> panelSize = { kWeaponPanelBackSize, kWeaponPanelFrontSize };
        const std::array<Vector4, 2> panelColor = { kWeaponPanelBackColor, kWeaponPanelFrontColor };
        const std::array<Vector4, 2> lineColor = { kWeaponPanelBackLineColor, kWeaponPanelFrontLineColor };
        for (size_t i = 0; i < weaponPanels.size(); ++i) {
            weaponPanels[i] = std::make_unique<Sprite>();
            weaponPanels[i]->Initialize(framework->GetSpriteCommon(), "white");
            weaponPanels[i]->SetAnchorPoint({ 1.0f, 1.0f });
            weaponPanels[i]->SetPosition(panelPos[i]);
            weaponPanels[i]->SetSize(panelSize[i]);
            weaponPanels[i]->SetColor(panelColor[i]);
            weaponPanels[i]->Update();

            weaponPanelLines[i] = std::make_unique<Sprite>();
            weaponPanelLines[i]->Initialize(framework->GetSpriteCommon(), "white");
            weaponPanelLines[i]->SetAnchorPoint({ 1.0f, 1.0f });
            weaponPanelLines[i]->SetPosition(panelPos[i]);
            weaponPanelLines[i]->SetSize({ panelSize[i].x, kWeaponPanelLineHeight });
            weaponPanelLines[i]->SetColor(lineColor[i]);
            weaponPanelLines[i]->Update();
        }

        weaponSlotTextFront.Initialize(framework->GetSpriteCommon(), 1);
        weaponSlotTextFront.SetScale(kWeaponSlotTextFrontScale);
        weaponSlotTextFront.SetPosition(kWeaponSlotTextFrontPos);
        weaponSlotTextFront.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        weaponSlotTextBack.Initialize(framework->GetSpriteCommon(), 1);
        weaponSlotTextBack.SetScale(kWeaponSlotTextBackScale);
        weaponSlotTextBack.SetPosition(kWeaponSlotTextBackPos);
        weaponSlotTextBack.SetColor(kWeaponSlotTextBackColor);
    }

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

    // --- 勝敗演出用の全画面オーバーレイ（仮：初期状態は透明） ---
    gameOverOverlay = std::make_unique<Sprite>();
    gameOverOverlay->Initialize(framework->GetSpriteCommon(), "white");
    gameOverOverlay->SetPosition({ 0.0f, 0.0f });
    gameOverOverlay->SetSize({ (float)WinApp::KclientWidth, (float)WinApp::KclientHeight });
    gameOverOverlay->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    gameOverText.Initialize(framework->GetSpriteCommon(), 4); // "LOSE"が最大4文字
    gameOverText.SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // --- 終了確認オーバーレイ（フェード無し、確認中は常に一定の暗さで即表示） ---
    quitConfirmOverlay = std::make_unique<Sprite>();
    quitConfirmOverlay->Initialize(framework->GetSpriteCommon(), "white");
    quitConfirmOverlay->SetPosition({ 0.0f, 0.0f });
    quitConfirmOverlay->SetSize({ (float)WinApp::KclientWidth, (float)WinApp::KclientHeight });
    quitConfirmOverlay->SetColor({ 0.0f, 0.0f, 0.0f, 0.6f });

    const char* quitMenuLabels[kQuitMenuCount] = { "TITLE", "QUIT" };
    for (int i = 0; i < kQuitMenuCount; ++i) {
        BitmapText& text = quitMenuTexts[i];
        text.Initialize(framework->GetSpriteCommon(), 5); // 最長"TITLE"の5文字
        text.SetText(quitMenuLabels[i]);
        text.SetPosition({ ((float)WinApp::KclientWidth - text.GetWidth()) / 2.0f, kGameOverTextPositionY + kQuitMenuLineSpacing * i });
    }

    // --- 開始演出（READY→FIGHT） ---
    introText.Initialize(framework->GetSpriteCommon(), 5); // "READY"/"FIGHT"が最大5文字
    introText.SetScale(kIntroTextScale);
    phaseText.Initialize(framework->GetSpriteCommon(), kPhaseTextMaxChars); // "PHASE 2"/"PHASE 3"の7文字
    phaseText.SetScale(kPhaseTextScale);
    phaseText.SetText("PHASE 2"); // 未設定のままUpdateしないよう初期文字を入れておく（移行時に差し替え）
    introBarTop = std::make_unique<Sprite>();
    introBarTop->Initialize(framework->GetSpriteCommon(), "white");
    introBarTop->SetPosition({ 0.0f, 0.0f });
    introBarTop->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    introBarBottom = std::make_unique<Sprite>();
    introBarBottom->Initialize(framework->GetSpriteCommon(), "white");
    introBarBottom->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    // StartIntro(true)はボス等の生成後に呼ぶ必要があるため、Initialize()の最後で呼ぶ

    std::unique_ptr<Camera> newCamera = std::make_unique<Camera>();
    CameraManager::GetInstance()->AddCamera("MainCamera", std::move(newCamera));
    camera = CameraManager::GetInstance()->GetActiveCamera();
    
    // z=-10だと南側中央カバー({0,1.0,-12}, half-extent{1.5,2.0,1.5} → z範囲[-13.5,-10.5])との
    // 距離がcollisionRadius(0.5)とちょうど境界一致し、接地判定がこのカバーの天面(y=3)を
    // 拾ってプレイヤーがブロックの上に立った状態でスポーンしてしまうため、余裕を持って離す
    playerPos = { 0.0f, 0.0f, -8.0f };
    camera->SetTranslate({ playerPos.x, playerPos.y + eyeHeight, playerPos.z });

    cubeModel = std::make_unique<Model>();
    cubeModel->Initialize(framework->GetDxCommon(), "Resources/cube", "cube.obj");

    skybox = std::make_unique<Skybox>();
    skybox->Initialize(framework->GetSkyboxCommon(), "Resources/skybox/sky_cubemap.dds");
    skybox->SetModel(cubeModel.get());

    floor = std::make_unique<Object3d>();
    floor->Initialize(framework->GetObject3dCommon());
    floor->SetModel(ModelManager::GetInstance()->LoadModel(ArenaLayout::kArenaModelDir, ArenaLayout::kFloorModelFile));
    floor->SetScale(ArenaLayout::kFloorScale);
    floor->SetTranslate(ArenaLayout::kFloorTranslate);
    floor->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // モデル自体に色が付いているので白（基本色を暗くしない）

    // 壁の配置データはTitleScene（3D背景）と共有するためArenaLayout.hにある
    for (const auto& data : ArenaLayout::kWallDatas) {
        auto wall = std::make_unique<Object3d>();
        wall->Initialize(framework->GetObject3dCommon());
        wall->SetModel(ModelManager::GetInstance()->LoadModel(ArenaLayout::kArenaModelDir, ArenaLayout::WallModelFile(data.kind)));
        wall->SetTranslate(data.pos);
        wall->SetScale(data.scale); // GetAABBがscaleをhalf-extentとして使うため、モデルは-1〜1正規化でscaleは変えない
        wall->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
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
        enemy.object->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/props", "target.glb"));
        enemy.object->SetTranslate(pos);
        enemy.object->SetScale({ 0.5f, 0.75f, 0.5f });
        enemy.object->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // モデル自体に色が付いている
        enemy.isActive = true;
        enemy.respawnTimer = 0.0f;
        enemies.push_back(std::move(enemy));
    }

    // ボス（1v1ボス戦フェーズ2：HP/UIの土台。移動・攻撃AIは次フェーズ）
    // 中央カバー(±12)・角プラットフォーム(±14)と被らない開けた位置に配置
    boss = std::make_unique<Object3d>();
    boss->Initialize(framework->GetObject3dCommon());
    bossModel = std::make_unique<Model>();
    bossModel->Initialize(framework->GetDxCommon(), "Resources/cube", "cubeSliced.obj");
    boss->SetModel(bossModel.get());
    // 底面が床(y=0)にちょうど乗るよう、bossPosの足元基準にkBossHalfHeightを足した高さに配置する
    // （前フェーズではy=1.5決め打ちで底面が1ユニット埋まっていたため、ここで修正）
    boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });
    boss->SetScale(ArenaLayout::kBossScale);
    boss->SetColor(ArenaLayout::kBossColor); // 芯は描画しないので色は見えない

    // 描画用の5パーツ。bossと同じ位置・回転・scaleで置くと組み上がる（transformはUpdateBossParts()でbossから写す）
    for (int i = 0; i < kBossPartCount; ++i) {
        bossParts[i] = std::make_unique<Object3d>();
        bossParts[i]->Initialize(framework->GetObject3dCommon());
        bossParts[i]->SetModel(ModelManager::GetInstance()->LoadModel(ArenaLayout::kBossModelDir, ArenaLayout::kBossPartFiles[i]));
        bossParts[i]->SetScale(ArenaLayout::kBossScale);
        bossParts[i]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }

    // ボスのレーザー（P2以降の薙ぎ払い）。位置・回転・スケールはボスAIが毎フレーム設定する
    bossLaser = std::make_unique<Object3d>();
    bossLaser->Initialize(framework->GetObject3dCommon());
    bossLaser->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/props", "laser.glb"));
    bossLaser->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    bossLaser->SetScale({ 0.0f, 0.0f, 0.0f });

    // P3のテレポート行き先マーカーと衝撃波リング（どちらも shockwave.glb。使うときにスケールを設定する）
    bossTeleportMarker = std::make_unique<Object3d>();
    bossTeleportMarker->Initialize(framework->GetObject3dCommon());
    bossTeleportMarker->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/props", "shockwave.glb"));
    bossTeleportMarker->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    bossTeleportMarker->SetScale({ 0.0f, 0.0f, 0.0f });

    bossShockwave = std::make_unique<Object3d>();
    bossShockwave->Initialize(framework->GetObject3dCommon());
    bossShockwave->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/props", "shockwave.glb"));
    bossShockwave->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    bossShockwave->SetScale({ 0.0f, 0.0f, 0.0f });

    // ボスの遠距離攻撃（グレネード）用のプール
    grenades.reserve(kMaxGrenades);
    for (int i = 0; i < kMaxGrenades; ++i) {
        Grenade grenade;
        grenade.object = std::make_unique<Object3d>();
        grenade.object->Initialize(framework->GetObject3dCommon());
        grenade.object->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/props", "grenade.glb"));
        grenade.object->SetScale({ 0.2f, 0.2f, 0.2f });
        grenade.object->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // モデル自体に色が付いている
        grenade.active = false;
        grenades.push_back(std::move(grenade));
    }

    portalModelA = ModelManager::GetInstance()->LoadModel("Resources/models/props", "portal_a.glb");
    portalModelB = ModelManager::GetInstance()->LoadModel("Resources/models/props", "portal_b.glb");

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

    // --- 視点武器（ショットガン本体＋先台。どちらも実寸モデルなのでscaleは1） ---
    weaponBody = std::make_unique<Object3d>();
    weaponBody->Initialize(framework->GetObject3dCommon());
    weaponBody->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/shotgun", "shotgun.glb"));
    weaponBody->SetScale({ 1.0f, 1.0f, 1.0f });
    weaponBody->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    weaponPump = std::make_unique<Object3d>();
    weaponPump->Initialize(framework->GetObject3dCommon());
    weaponPump->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/shotgun", "shotgun_pump.glb"));
    weaponPump->SetScale({ 1.0f, 1.0f, 1.0f });
    weaponPump->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    weaponPortalGun = std::make_unique<Object3d>();
    weaponPortalGun->Initialize(framework->GetObject3dCommon());
    weaponPortalGun->SetModel(ModelManager::GetInstance()->LoadModel("Resources/models/rifle", "rifle.glb"));
    weaponPortalGun->SetScale({ 1.0f, 1.0f, 1.0f });
    weaponPortalGun->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    // --- 設定メニュー（ImGui非依存。Pキーでのポーズ中にキー操作で調整できる） ---
    {
        ScreenEffectController* pp = screenEffect;
        std::vector<PauseTuningMenu::Item> menuItems;

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

    // 開始演出（フル版）。UpdateIntroVisual()がboss/screenEffectを触るため、全オブジェクトの生成後に呼ぶ
    StartIntro(true);
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

    // --- ESCメニュー中は他の更新を全て止め、↑↓(選択)/Enter(決定)/ESC(キャンセル)の入力のみ受け付ける ---
    if (quitConfirming) {
        if (input->TriggerKey(DIK_UP) || input->TriggerKey(DIK_W)) {
            quitMenuIndex = (quitMenuIndex + kQuitMenuCount - 1) % kQuitMenuCount;
        }
        if (input->TriggerKey(DIK_DOWN) || input->TriggerKey(DIK_S)) {
            quitMenuIndex = (quitMenuIndex + 1) % kQuitMenuCount;
        }
        if (input->TriggerKey(DIK_RETURN)) {
            // 暗転してから実行（暗転中はSceneManagerがUpdateを止める）
            if (quitMenuIndex == kQuitMenuTitle) {
                MyGame::GetInstance()->RequestTransition("TITLE", SceneTransition::kReturnTitleParams);
            } else {
                MyGame::GetInstance()->RequestFade(SceneTransition::kQuitParams, []() { Framework::GetInstance()->RequestEnd(); });
            }
        } else if (input->TriggerKey(DIK_ESCAPE)) {
            quitConfirming = false;
            // 確認画面を開く前の状態（プレイ中）に戻す。ポーズ中・ゲームオーバー中に開いた場合はマウス表示のまま
            if (!isLookPaused && !gameOver) {
                winApp->ShowCursor(false);
                winApp->SetClipCursor(true);
            }
        }
        quitConfirmOverlay->Update();
        for (int i = 0; i < kQuitMenuCount; ++i) {
            quitMenuTexts[i].SetColor(i == quitMenuIndex ? kQuitMenuSelectedColor : kQuitMenuUnselectedColor);
            quitMenuTexts[i].Update();
        }
        return;
    }

    // --- ESCキーで終了確認を開始（プレイ中・設定メニュー中・ゲームオーバー中いずれからでも） ---
    if (input->TriggerKey(DIK_ESCAPE)) {
        quitConfirming = true;
        quitMenuIndex = kQuitMenuTitle; // 開くたびに誤って終了しにくい方を初期選択にする
        winApp->ShowCursor(true);
        winApp->SetClipCursor(false);
        return;
    }

    // --- 勝敗が決した後は完全に停止し、オーバーレイのフェードインだけ進める ---
    if (gameOver) {
        gameOverFadeTimer = (std::min)(gameOverFadeTimer + 1.0f / 60.0f, kGameOverFadeDuration);
        float alpha = (gameOverFadeTimer / kGameOverFadeDuration) * 0.6f;
        const Vector3& resultColor = playerWon ? kWinOverlayColor : kLoseOverlayColor;
        gameOverOverlay->SetColor({ resultColor.x, resultColor.y, resultColor.z, alpha });
        gameOverOverlay->Update();
        gameOverText.Update();

        // フェードが終わってから（演出の途中で誤爆しないように）Enterキーでリスタート
        if (gameOverFadeTimer >= kGameOverFadeDuration && input->TriggerKey(DIK_RETURN)) {
            // 結果色を暗くした幕で暗転し、真っ暗な間にリスタートする
            TransitionParams retryParams = SceneTransition::kRetryParams;
            retryParams.color = { resultColor.x * kRetryFadeColorScale, resultColor.y * kRetryFadeColorScale, resultColor.z * kRetryFadeColorScale, 1.0f };
            MyGame::GetInstance()->RequestFade(retryParams, [this]() { RestartGame(); });
        }
        return;
    }

    // --- マウス操作（プレイ中のみ） ---
    if (!isLookPaused) {
        Input::MouseMove mouseMove = input->GetMouseMove();
        // フル版の開始演出（READY中）は視点をボスに向けたまま固定する
        bool isIntroViewLocked = (introPhase == IntroPhase::kReady && introIsFull);
        if (!isIntroViewLocked) {
            cameraTransform.rotate.y += (float)mouseMove.lX * mouseSensitivity;
            cameraTransform.rotate.x += (float)mouseMove.lY * mouseSensitivity;
        }

        // マウスを中央に固定し続ける（ウィンドウの実際のクライアント領域の中央。ウィンドウサイズやフルスクリーンで変わる）
        RECT clientRect{};
        GetClientRect(winApp->GetHwnd(), &clientRect);
        POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
        ClientToScreen(winApp->GetHwnd(), &center);
        SetCursorPos(center.x, center.y);
    }

    // --- グレースケール表示の切り替え（Gキー） ---
    if (input->TriggerKey(DIK_G)) {
        screenEffect->ToggleGrayscale();
    }

    // --- セピア表示の切り替え（Hキー。回想シーン風の演出用） ---
    if (input->TriggerKey(DIK_H)) {
        screenEffect->ToggleSepia();
    }

    // --- 画面全体ぼかしの切り替え（Bキー） ---
    if (input->TriggerKey(DIK_B)) {
        screenEffect->ToggleBlur();
    }

    // --- 画面周辺減光（ヴィネット）の切り替え（Vキー） ---
    if (input->TriggerKey(DIK_V)) {
        screenEffect->ToggleVignette();
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

    // --- ポーズ中はゲーム世界の時間を止める ---
    // 移動・ボスAI・グレネード・タイマー類・パーティクルは一切進めず、設定メニューの操作と
    // 描画に必要な行列・UIの更新だけを行う（設定を触っている間にボスに殴られるのを防ぐため）
    if (isLookPaused) {
        settingsMenu.Update(input, isLookPaused);
        UpdateRenderObjects();
        UpdateDebugGui();
        return;
    }

    // --- 開始演出のREADY中：移動・射撃・ボスAIは止め、視点操作と描画に必要な更新だけ行う ---
    if (introPhase == IntroPhase::kReady) {
        introElapsed += 1.0f / 60.0f;
        float readyDuration = introIsFull ? kIntroFullReadyDuration : kIntroReadyDuration;
        if (introElapsed >= readyDuration) {
            // FIGHT表示と同時に操作解禁（次フレームから通常の更新に入る）
            introPhase = IntroPhase::kFight;
            introElapsed = 0.0f;
            SetIntroText("FIGHT", kIntroFightColor);
            TriggerCameraShake(kIntroFightShakeDuration, kIntroFightShakeStrength);
        }
        UpdateIntroVisual(); // UpdateRenderObjects()より前に置き、HPバーのチャージ倍率を当フレームから反映する
        UpdateIntroCamera();
        camera->Update();
        UpdateWeaponViewModel(); // 銃のWVPも更新しないと単位行列のまま描画されるため必須
        UpdateRenderObjects();
        UpdateDebugGui();
        return;
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

    // ボスの近接突進がヒットした時のノックバックを、減衰させながら移動方向へ加算する（上と同じ方式）
    if (knockbackVelocityTimer > 0.0f) {
        moveDir += knockbackVelocity * (knockbackVelocityTimer / kKnockbackDuration);
        knockbackVelocityTimer -= 1.0f / 60.0f;
        if (knockbackVelocityTimer < 0.0f) knockbackVelocityTimer = 0.0f;
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

    // --- ボスの攻撃AI／移動AI ---
    // playerPosがポータル移動も含めて確定した後（UpdatePortalTeleport済み）に計算する。
    // kChasing中は既存の追跡・障害物回避ロジックで動き続けながらbossAttackTimerを減算し、
    // 0以下になったらその時点の距離で近接突進／遠距離グレネードを自動選択する
    // （＝クールダウン中もボスは棒立ちにならず、既存の追跡AIをそのまま活かす設計）
    {
        Vector3 toPlayer = { playerPos.x - bossPos.x, 0.0f, playerPos.z - bossPos.z };
        float distance = Calculation::Length(toPlayer);
        Vector3 dirToPlayer = (distance > 0.0001f) ? Calculation::Normalize(toPlayer) : Vector3{ 0.0f, 0.0f, 1.0f };

        switch (bossAttackState) {
        case BossAttackState::kChasing: {
            if (distance > kBossStopDistance) {
                float distanceT = std::clamp((distance - kBossStopDistance) / (kBossFarDistance - kBossStopDistance), 0.0f, 1.0f);
                float baseSpeed = kBossSpeedNear + (kBossSpeedFar - kBossSpeedNear) * distanceT;

                float hpRatio = bossHP / kBossMaxHP;
                float enrageMultiplier = 1.0f + (kBossEnrageMaxMultiplier - 1.0f) * (1.0f - hpRatio);

                float bossMoveSpeed = baseSpeed * enrageMultiplier;
                if (bossPhase == BossPhase::kP3) bossMoveSpeed *= kBossPhase3SpeedMultiplier;
                Vector3 dir = dirToPlayer;
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
            } else {
                // 間合いに入って停止中も、常にプレイヤーの方を向く
                boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            }

            // 攻撃判定：クールダウン(bossAttackTimer)が明けたら、距離しきい値で近接突進／遠距離グレネードを自動選択する
            if (bossAttackTimer > 0.0f) {
                bossAttackTimer -= 1.0f / 60.0f;
            } else if (distance <= kBossMeleeRange) {
                // P3の近距離は、一定確率で衝撃波リング。それ以外は突進
                bool useShockwave = false;
                if (bossPhase == BossPhase::kP3) {
                    std::random_device seedGen;
                    std::mt19937 randomEngine(seedGen());
                    std::uniform_real_distribution<float> distChance(0.0f, 1.0f);
                    useShockwave = distChance(randomEngine) < kBossP3ShockwaveChance;
                }
                if (useShockwave) {
                    bossAttackState = BossAttackState::kShockwaveWindup;
                    bossAttackTimer = kBossShockwaveWindupDuration;
                    bossWarningEmissive = { 1.0f, 0.35f, 0.05f }; // 衝撃波予備動作：警告色（橙赤）
                } else {
                    bossAttackState = BossAttackState::kMeleeWindup;
                    bossAttackTimer = kBossMeleeWindupDuration;
                    bossWarningEmissive = { 1.0f, 0.15f, 0.05f }; // 突進予備動作：警告色（赤）。くさびの発光で表現する
                }
            } else {
                // P2以降の遠距離は、一定確率でレーザー薙ぎ払い・テレポートを選ぶ（残りはグレネード）
                bool useLaser = false;
                bool useTeleport = false;
                if (bossPhase != BossPhase::kP1) {
                    std::random_device seedGen;
                    std::mt19937 randomEngine(seedGen());
                    std::uniform_real_distribution<float> distChance(0.0f, 1.0f);
                    float roll = distChance(randomEngine);
                    useLaser = roll < kBossFarLaserChance;
                    useTeleport = !useLaser && roll < kBossFarLaserChance + kBossFarTeleportChance;
                }
                // テレポートの行き先が見つからなければ、やめてグレネードにする
                if (useTeleport && !TryPickBossTeleportDest()) {
                    useTeleport = false;
                }
                if (useTeleport) {
                    bossAttackState = BossAttackState::kTeleportOut;
                    bossAttackTimer = kBossTeleportOutDuration;
                    bossTeleportMarkerVisible = true;
                } else if (useLaser) {
                    bossAttackState = BossAttackState::kLaserWindup;
                    bossAttackTimer = kBossLaserWindupDuration;
                    bossWarningEmissive = kBossLaserWarningEmissive; // レーザー予備動作：警告色（紫）
                    bossLaserCenterYaw = std::atan2(dirToPlayer.x, dirToPlayer.z); // 薙ぎの中心はここで固定
                    bossLaserHasHit = false;
                } else {
                    bossAttackState = BossAttackState::kRangedWindup;
                    bossAttackTimer = kBossRangedWindupDuration;
                    bossWarningEmissive = { 1.0f, 0.5f, 0.05f }; // 投擲予備動作：警告色（橙）
                }
            }
            break;
        }
        case BossAttackState::kMeleeWindup: {
            // 予備動作中はプレイヤーの方を向いたまま停止し、突進の狙いを最後まで追従させる
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            if (bossAttackTimer <= 0.0f) {
                bossDashDir = dirToPlayer; // 突進方向はここで固定し、以後は追尾しない
                bossAttackState = BossAttackState::kMeleeDash;
                bossAttackTimer = kBossMeleeDashDuration;
                bossDashCurrentSpeed = kBossDashStartSpeed; // 突進開始時は遅めから、加速しながら距離を詰める
                bossWarningEmissive = { 0.0f, 0.0f, 0.0f }; // 通常に戻す
            }
            break;
        }
        case BossAttackState::kMeleeDash: {
            const float kGroundOffset = 0.05f;
            bossDashCurrentSpeed = (std::min)(bossDashCurrentSpeed + kBossDashAcceleration, kBossDashMaxSpeed);

            Vector3 nextBossPos = bossPos;
            nextBossPos.x += bossDashDir.x * bossDashCurrentSpeed;
            nextBossPos.z += bossDashDir.z * bossDashCurrentSpeed;
            Vector3 checkBossPos = nextBossPos;
            checkBossPos.y += kGroundOffset;

            bool hitWall = CheckCollision(checkBossPos, nullptr, kBossCollisionRadius);
            if (!hitWall) {
                bossPos = nextBossPos;
            }

            float hitDistance = Calculation::Length(Vector3{ playerPos.x - bossPos.x, 0.0f, playerPos.z - bossPos.z });
            bool hitPlayer = hitDistance <= (kBossCollisionRadius + collisionRadius);

            bossAttackTimer -= 1.0f / 60.0f;

            if (hitPlayer) {
                TakeDamage(kBossMeleeDamage);
                // ノックバック方向はボス→プレイヤー方向（ユーザー指定）
                Vector3 knockDir = (hitDistance > 0.0001f)
                    ? Vector3{ (playerPos.x - bossPos.x) / hitDistance, 0.0f, (playerPos.z - bossPos.z) / hitDistance }
                    : bossDashDir;
                knockbackVelocity = { knockDir.x * kKnockbackSpeed, 0.0f, knockDir.z * kKnockbackSpeed };
                knockbackVelocityTimer = kKnockbackDuration;

                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            } else if (hitWall || bossAttackTimer <= 0.0f) {
                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            }
            break;
        }
        case BossAttackState::kRangedWindup: {
            // 予備動作中はプレイヤーの方を向いたまま停止する
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            if (bossAttackTimer <= 0.0f) {
                bossWarningEmissive = { 0.0f, 0.0f, 0.0f }; // 通常に戻す
                bossGrenadeBurstRemaining = kBossGrenadeBurstCount;
                bossGrenadeBurstTimer = 0.0f; // 1発目は即発射
                bossAttackState = BossAttackState::kRangedBurst;
            }
            break;
        }
        case BossAttackState::kRangedBurst: {
            // 3連射の間もプレイヤーを狙い続ける
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossGrenadeBurstTimer -= 1.0f / 60.0f;
            if (bossGrenadeBurstTimer <= 0.0f) {
                FireGrenade();
                --bossGrenadeBurstRemaining;
                if (bossGrenadeBurstRemaining > 0) {
                    bossGrenadeBurstTimer = kBossGrenadeBurstInterval;
                } else {
                    bossAttackState = BossAttackState::kChasing;
                    bossAttackTimer = BossAttackCooldown();
                }
            }
            break;
        }
        case BossAttackState::kPhaseShift: {
            // 移行演出中は動かず攻撃もせず、プレイヤーの方を向くだけ（DamageBossは無敵）
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            float elapsed = kBossPhaseShiftDuration - bossAttackTimer;
            if (bossAttackTimer <= 0.0f) {
                screenEffect->SetTransitionGlitch(0.0f, elapsed); // 確実に切る
                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            } else {
                float glitch = kBossPhaseShiftGlitch * (1.0f - Easing::OutCubic(Easing::Clamp01(elapsed / kBossPhaseShiftGlitchDuration)));
                screenEffect->SetTransitionGlitch(glitch, elapsed);
            }
            break;
        }
        case BossAttackState::kLaserWindup: {
            // 停止してプレイヤーの方を向く。薙ぎ始めの向きに細いレーザーを予兆として出す
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            UpdateBossLaserTransform(bossLaserCenterYaw - kBossLaserSweepHalfAngle, kBossLaserTelegraphRadius);
            bossLaserVisible = true;
            bossAttackTimer -= 1.0f / 60.0f;
            if (bossAttackTimer <= 0.0f) {
                bossWarningEmissive = { 0.0f, 0.0f, 0.0f }; // 通常に戻す
                bossAttackState = BossAttackState::kLaserSweep;
                bossAttackTimer = kBossLaserSweepDuration;
            }
            break;
        }
        case BossAttackState::kLaserSweep: {
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            float sweepT = Easing::Clamp01(1.0f - bossAttackTimer / kBossLaserSweepDuration);
            float startYaw = bossLaserCenterYaw - kBossLaserSweepHalfAngle;
            float endYaw = bossLaserCenterYaw + kBossLaserSweepHalfAngle;
            float yaw = startYaw + (endYaw - startYaw) * Easing::InOutSine(sweepT);
            UpdateBossLaserTransform(yaw, kBossLaserRadius);
            bossLaserVisible = true;

            // 当たり判定：足元が低く、XZ平面でレーザー線分との距離が半径+プレイヤー半径以下なら命中（1回の薙ぎで1回だけ）
            if (!bossLaserHasHit && playerPos.y < kBossLaserHeight + kBossLaserRadius) {
                Vector2 segDir = { std::sin(yaw), std::cos(yaw) };
                Vector2 rel = { playerPos.x - bossPos.x, playerPos.z - bossPos.z };
                float along = std::clamp(rel.x * segDir.x + rel.y * segDir.y, 0.0f, kBossLaserLength);
                float distX = rel.x - segDir.x * along;
                float distZ = rel.y - segDir.y * along;
                float laserDistance = std::sqrt(distX * distX + distZ * distZ);
                if (laserDistance <= kBossLaserRadius + collisionRadius) {
                    TakeDamage(kBossLaserDamage);
                    bossLaserHasHit = true;
                }
            }

            if (bossAttackTimer <= 0.0f) {
                StopBossLaser();
                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            }
            break;
        }
        case BossAttackState::kTeleportOut: {
            // その場で止まってプレイヤーの方を向き、上から消える。行き先には予兆のマーカーが出ている
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            float elapsed = kBossTeleportOutDuration - bossAttackTimer;
            float t = Easing::Clamp01(elapsed / kBossTeleportOutDuration);
            // 境界の発光帯の分だけ余分に動かし、t=1で完全に消えるようにする
            float bossHeight = kBossHalfHeight * 2.0f + kIntroMaterializeEdgeWidth;
            SetBossDissolve(bossPos.y + bossHeight * (1.0f - Easing::InCubic(t)), kIntroMaterializeEdgeWidth, elapsed, 1.0f);
            screenEffect->SetTransitionGlitch(kBossTeleportGlitch * t, elapsed);
            if (bossAttackTimer <= 0.0f) {
                bossPos = bossTeleportDest; // 行き先へ確定（足元基準）
                bossAttackState = BossAttackState::kTeleportIn;
                bossAttackTimer = kBossTeleportInDuration;
                boss->SetRotate({ 0.0f, std::atan2(playerPos.x - bossPos.x, playerPos.z - bossPos.z), 0.0f });
            }
            break;
        }
        case BossAttackState::kTeleportIn: {
            // 行き先で下から現れる。ブロック欠けは現れ切るにつれて弱める
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            float elapsed = kBossTeleportInDuration - bossAttackTimer;
            float t = Easing::Clamp01(elapsed / kBossTeleportInDuration);
            if (bossAttackTimer <= 0.0f) {
                StopBossTeleport();
                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            } else {
                float bossHeight = kBossHalfHeight * 2.0f + kIntroMaterializeEdgeWidth;
                SetBossDissolve(bossPos.y + bossHeight * Easing::OutCubic(t), kIntroMaterializeEdgeWidth, elapsed, 1.0f - t);
                screenEffect->SetTransitionGlitch(kBossTeleportGlitch * (1.0f - t), elapsed);
            }
            break;
        }
        case BossAttackState::kShockwaveWindup: {
            // 止まってプレイヤーの方を向き、橙赤の警告を出す。終わったら衝撃波を発生させる
            boss->SetRotate({ 0.0f, std::atan2(dirToPlayer.x, dirToPlayer.z), 0.0f });
            bossAttackTimer -= 1.0f / 60.0f;
            if (bossAttackTimer <= 0.0f) {
                bossWarningEmissive = { 0.0f, 0.0f, 0.0f };
                shockwaveActive = true;
                shockwaveHasHit = false;
                shockwaveCenter = { bossPos.x, 0.0f, bossPos.z };
                shockwaveRadius = kBossShockwaveStartRadius;
                TriggerCameraShake(kBossShockwaveShakeDuration, kBossShockwaveShakeStrength);
                bossAttackState = BossAttackState::kChasing;
                bossAttackTimer = BossAttackCooldown();
            }
            break;
        }
        }

        if (bossPhase == BossPhase::kP3) {
            bossPhase3PulseTimer += 1.0f / 60.0f; // P3の発光の脈打ち用
        }

        boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });

        // くさびの開閉：P1は 追跡0・それ以外1。P2以降は 追跡中kBossPhase2IdleOpen・攻撃中1
        float openTarget = 1.0f;
        if (bossAttackState == BossAttackState::kChasing) {
            openTarget = (bossPhase == BossPhase::kP1) ? 0.0f : kBossPhase2IdleOpen;
        }
        float openStep = kBossOpenSpeed / 60.0f;
        if (bossOpenAmount < openTarget) {
            bossOpenAmount = (std::min)(bossOpenAmount + openStep, openTarget);
        } else {
            bossOpenAmount = (std::max)(bossOpenAmount - openStep, openTarget);
        }
        if (bossHitFlashTimer > 0.0f) {
            bossHitFlashTimer = (std::max)(bossHitFlashTimer - 1.0f / 60.0f, 0.0f);
        }
        if (phaseTextTimer > 0.0f) {
            phaseTextTimer = (std::max)(phaseTextTimer - 1.0f / 60.0f, 0.0f);
        }
    }

    // --- P3の衝撃波リングの更新（広がりながら、ジャンプで避けられる低い輪として当たり判定を持つ） ---
    if (shockwaveActive) {
        shockwaveRadius += kBossShockwaveSpeed / 60.0f;
        if (shockwaveRadius > kBossShockwaveMaxRadius) {
            StopBossShockwave();
        } else if (!shockwaveHasHit && playerPos.y < kBossShockwaveHeight) {
            float ringDistance = Calculation::Length(Vector3{ playerPos.x - shockwaveCenter.x, 0.0f, playerPos.z - shockwaveCenter.z });
            if (std::abs(ringDistance - shockwaveRadius) <= kBossShockwaveHitHalfWidth + collisionRadius) {
                TakeDamage(kBossShockwaveDamage);
                shockwaveHasHit = true;
            }
        }
    }

    // --- ボスのグレネードの更新（重力を受けながら飛び、床・壁に着弾すると爆発する） ---
    for (auto& grenade : grenades) {
        if (!grenade.active) continue;

        grenade.velocity.y += kGrenadeGravity;
        grenade.position.x += grenade.velocity.x;
        grenade.position.y += grenade.velocity.y;
        grenade.position.z += grenade.velocity.z;

        if (CheckCollision(grenade.position, nullptr, kGrenadeRadius) || grenade.position.y < -5.0f) {
            ExplodeGrenade(grenade);
            continue;
        }

        grenade.object->SetTranslate(grenade.position);
        grenade.object->Update();
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

    // --- 射撃間隔クールダウンの減衰 ---
    if (fireCooldownTimer > 0.0f) {
        fireCooldownTimer -= 1.0f / 60.0f;
        if (fireCooldownTimer < 0.0f) fireCooldownTimer = 0.0f;
    }

    // --- 射撃処理（ライフルモードの時のみ。ポータルガンモードの左右クリックは上で処理済み） ---
    if (!isLookPaused && weaponMode == WeaponMode::kRifle && fireCooldownTimer <= 0.0f && input->TriggerMouseButton(0)) {
        FireShot();
        fireCooldownTimer = kFireCooldownDuration;
    }

    // 視点武器の追従・リコイル更新（FireShot後に置き、当フレームからキックを反映する）
    UpdateWeaponViewModel();

    // カメラシェイクで加算したオフセットを差し引き、次フレームのマウス視点計算のベース値を汚さないようにする
    cameraTransform.rotate.x -= shakeOffsetX;
    cameraTransform.rotate.y -= shakeOffsetY;

    settingsMenu.Update(input, isLookPaused);
    particleManagerLine->Update();
    particleManagerPoint->Update();
    particleManagerTriangle->Update();
    UpdateRenderObjects();

    // --- 勝敗判定：HPが0になった瞬間に一度だけ確定させる ---
    if (playerHP <= 0.0f) {
        gameOver = true;
        playerWon = false;
        gameOverText.SetText("LOSE");
    } else if (bossHP <= 0.0f) {
        gameOver = true;
        playerWon = true;
        gameOverText.SetText("WIN");
    }
    if (gameOver) {
        StopBossLaser(); // 決着後はレーザーを消す
        StopBossTeleport();
        StopBossShockwave();
        phaseTextTimer = 0.0f;
        // 文字数に応じて画面中央に揃える（"WIN"=3文字/"LOSE"=4文字）
        float textWidth = kGameOverTextCharWidth * (playerWon ? 3.0f : 4.0f);
        gameOverText.SetPosition({ ((float)WinApp::KclientWidth - textWidth) / 2.0f, kGameOverTextPositionY });
    }

    // --- 開始演出のFIGHT表示：操作は解禁済み、文字を徐々に消し、黒帯を引いていく ---
    if (introPhase == IntroPhase::kFight) {
        introElapsed += 1.0f / 60.0f;
        if (introElapsed >= kIntroFightDuration) {
            introPhase = IntroPhase::kNone;
        } else {
            UpdateIntroVisual();
        }
    }

    UpdateDebugGui();
}

void GameScene::UpdateRenderObjects() {
    skybox->Update();
    floor->Update();
    for (auto& wall : walls) { wall->Update(); }
    for (auto& enemy : enemies) { enemy.object->Update(); }
    boss->Update();
    UpdateBossParts(); // bossのtranslate/rotate/scaleが確定した後に5パーツへ写す
    bossLaser->Update(); // 位置・回転・スケールはボスAIが設定済み。呼び忘れると単位行列のまま巨大に描画される
    // マーカー・衝撃波の位置とスケールは表示中だけ更新側で毎フレーム設定する（Updateを忘れると巨大に描画される）
    if (bossTeleportMarkerVisible) {
        bossTeleportMarker->SetTranslate({ bossTeleportDest.x, 0.0f, bossTeleportDest.z });
        bossTeleportMarker->SetScale({ kBossTeleportMarkerRadius, kBossTeleportMarkerHeight, kBossTeleportMarkerRadius });
    }
    bossTeleportMarker->Update();
    if (shockwaveActive) {
        bossShockwave->SetTranslate(shockwaveCenter);
        bossShockwave->SetScale({ shockwaveRadius, kBossShockwaveHeight, shockwaveRadius });
    }
    bossShockwave->Update();
    UpdatePhaseText();
    for (auto& decal : bulletDecals) { decal->Update(); }
    // 過去に敵ターゲットでUpdate()呼び忘れによりWVP行列が更新されず巨大に固定表示された
    // バグがあったため、ポータルの見た目も他のObject3dと同様に毎フレームUpdate()を呼ぶ
    if (portalA.visual) portalA.visual->Update();
    if (portalB.visual) portalB.visual->Update();
    // ポータルを置けなかった直後は、レティクルを一瞬灰色にする
    if (portalRejectTimer > 0.0f) {
        portalRejectTimer = (std::max)(portalRejectTimer - 1.0f / 60.0f, 0.0f);
    }
    reticle->SetColor(portalRejectTimer > 0.0f ? kReticleRejectColor : kReticleColor);
    reticle->Update();

    // HPバーの残量表示：現在HP比率に応じて塗りつぶし幅だけを縮める（背景は常にフル幅のまま）
    float hpRatio = playerHP / kPlayerMaxHP;
    hpBarFill->SetSize({ kHpBarMaxWidth * hpRatio, kHpBarHeight });
    hpBarBackground->Update();
    hpBarFill->Update();
    playerHpText.SetText(std::to_string(static_cast<int>(playerHP)));
    playerHpText.Update();

    UpdateWeaponHud();

    // ボスHPバーの残量表示：現在HP比率に応じて塗りつぶし幅だけを縮める
    float bossHpRatio = (bossHP / kBossMaxHP) * bossHpBarIntroScale;
    bossHpBarFill->SetSize({ kBossHpBarMaxWidth * bossHpRatio, kBossHpBarHeight });
    bossHpBarBackground->Update();
    bossHpBarFill->Update();
}

void GameScene::UpdateDebugGui() {
#ifdef _DEBUG
    Framework* framework = Framework::GetInstance();
    WinApp* winApp = framework->GetWinApp();
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
    // 影パスは深度のみでピクセルシェーダーの切り取りが効かないため、実体化中は影を落とさない
    if (boss && !IsBossMaterializing()) {
        for (auto& part : bossParts) { part->DrawShadow(lightViewProjection); }
    }
    for (auto& grenade : grenades) { if (grenade.active) grenade.object->DrawShadow(lightViewProjection); }
    object3dCommon->PostDrawShadow();

    // --- 1. 3D の描画 ---
    // 描画先は MyGame::Draw の RenderScaler::BeginScene が決めている（画面の効果があるときは別の描画先。
    // 画面への書き戻しと効果は SceneManager::Draw の RenderScaler::EndScene が行う）。
    // 影のパスで描画先が変わったので、メインの描画先に戻す
    framework->GetDxCommon()->BindMainRenderTarget();

    // スカイボックスの描画
    framework->GetSkyboxCommon()->PreDraw();
    skybox->Draw();

    // 3Dオブジェクトの描画
    framework->GetObject3dCommon()->PreDraw();
    floor->Draw();
    for (auto& wall : walls) { wall->Draw(); }
    for (auto& enemy : enemies) { if (enemy.isActive) enemy.object->Draw(); }
    if (boss) {
        for (auto& part : bossParts) { part->Draw(); }
        if (bossLaserVisible) bossLaser->Draw(); // 影は落とさない
        if (bossTeleportMarkerVisible) bossTeleportMarker->Draw();
        if (shockwaveActive) bossShockwave->Draw();
    }
    for (auto& grenade : grenades) { if (grenade.active) grenade.object->Draw(); }
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

    // --- 視点武器（常に最前面に描画。壁などにめり込んで消えないようにする）。開始演出の演出用カメラ中は隠す ---
    if (!IsIntroCameraActive()) {
        DrawWeaponViewModel();
    }

    // --- 3. UI（レティクル）の描画 ---
    // スプライトは描画キューに積むだけ。SceneManager が 3D を画面へ写した後に、積んだ順（同じレイヤー内）で描く
    if (!IsIntroCameraActive()) {
        reticle->Draw();

        // 武器表示：後ろ（グレースケール）→手前（カラー）の順に積んで、手前が上に重なるようにする
        bool isPortal = (weaponMode == WeaponMode::kPortalGun);
        weaponPanels[0]->Draw();
        weaponPanelLines[0]->Draw();
        weaponIcons[isPortal ? kIconShotgunGray : kIconPortalGunGray]->Draw();
        weaponSlotTextBack.Draw();
        weaponPanels[1]->Draw();
        weaponPanelLines[1]->Draw();
        if (isPortal) {
            weaponIcons[kIconPortalGun]->Draw();
        } else {
            // グレースケールの上に、クールタイムの明けた分だけカラーを重ねる（UpdateWeaponHud参照）
            weaponIcons[kIconShotgunGray]->Draw();
            weaponIcons[kIconShotgun]->Draw();
        }
        weaponSlotTextFront.Draw();
    }

    // --- 4. UI（プレイヤーHPバー）の描画 ---
    hpBarBackground->Draw();
    hpBarFill->Draw();
    playerHpText.Draw();

    // --- 4.5 UI（開始演出の黒帯）：プレイヤーHPは隠し、ボスHPバーはチャージが見えるよう帯の上に描く ---
    if (introPhase != IntroPhase::kNone && introIsFull) {
        introBarTop->Draw();
        introBarBottom->Draw();
    }

    // --- 5. UI（ボスHPバー）の描画 ---
    bossHpBarBackground->Draw();
    bossHpBarFill->Draw();

    // --- 5.5 UI（開始演出のREADY/FIGHT文字） ---
    if (introPhase != IntroPhase::kNone) {
        introText.Draw();
    }

    // --- 5.6 UI（フェーズ移行の"PHASE 2"/"PHASE 3"文字） ---
    if (phaseTextTimer > 0.0f) {
        phaseText.Draw();
    }

    // --- 6. UI（設定メニュー）の描画：Pキーでのポーズ中のみ表示 ---
    if (isLookPaused) {
        settingsMenu.Draw();
    }

    // --- 7. UI（勝敗演出の全画面オーバーレイ）：決着後のみ、フェードインしながら表示 ---
    if (gameOver) {
        gameOverOverlay->Draw();
        gameOverText.Draw();
    }

    // --- 8. UI（終了確認オーバーレイ）：ESCキーで確認中のみ最前面に表示 ---
    if (quitConfirming) {
        quitConfirmOverlay->Draw();
        for (auto& text : quitMenuTexts) { text.Draw(); }
    }
}

void GameScene::Finalize() {
    // このシーンのカメラを残すと、次に GameScene に入ったときの AddCamera が重複になるため、必ず外す
    CameraManager::GetInstance()->RemoveCamera("MainCamera");
    camera = nullptr;
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
    screenEffect->TriggerDamageVignette();
}

void GameScene::DamageBoss(float amount) {
    // フェーズ移行演出中は無敵
    if (bossAttackState == BossAttackState::kPhaseShift
        || bossAttackState == BossAttackState::kTeleportOut
        || bossAttackState == BossAttackState::kTeleportIn) {
        return;
    }
    bossHP -= amount;
    if (bossHP < 0.0f) {
        bossHP = 0.0f;
    }
    // 生存していて閾値を下回ったら次のフェーズへ（1発で複数またいでも1段ずつ）
    if (bossHP > 0.0f) {
        float hpRatio = bossHP / kBossMaxHP;
        if (bossPhase == BossPhase::kP1 && hpRatio < kBossPhase2HpRatio) {
            StartBossPhaseShift(BossPhase::kP2);
        } else if (bossPhase == BossPhase::kP2 && hpRatio < kBossPhase3HpRatio) {
            StartBossPhaseShift(BossPhase::kP3);
        }
    }
}

void GameScene::StartBossPhaseShift(BossPhase next) {
    bossPhase = next;
    bossAttackState = BossAttackState::kPhaseShift;
    bossAttackTimer = kBossPhaseShiftDuration;
    bossWarningEmissive = { 0.0f, 0.0f, 0.0f };
    StopBossLaser(); // 進行中のレーザーは止める（グレネードは飛んでいるものをそのまま）
    // テレポート中なら打ち切る。無敵なので移行は起きないが念のため、位置は現在のbossPosのまま（Out中は元の位置）で確定させる。
    // 衝撃波の予備動作はstateがkPhaseShiftに上書きされるので打ち切られる。広がっている衝撃波は消さない
    StopBossTeleport();
    bossGrenadeBurstRemaining = 0; // 3連射の途中だった場合の残弾を捨てる

    TriggerCameraShake(kBossPhaseShiftShakeDuration, kBossPhaseShiftShakeStrength);

    phaseText.SetText(next == BossPhase::kP2 ? "PHASE 2" : "PHASE 3");
    phaseText.SetPosition({ ((float)WinApp::KclientWidth - phaseText.GetWidth()) / 2.0f, kPhaseTextPositionY });
    phaseTextTimer = kPhaseTextDuration;
}

void GameScene::UpdatePhaseText() {
    // 最後のkPhaseTextFadeDurationでアルファを下げて消す（タイマーの減算はボスAI側＝ポーズ中は進まない）
    float alpha = (std::min)(phaseTextTimer / kPhaseTextFadeDuration, 1.0f);
    phaseText.SetColor({ kPhaseTextColor.x, kPhaseTextColor.y, kPhaseTextColor.z, alpha });
    phaseText.Update();
}

void GameScene::UpdateBossLaserTransform(float yaw, float radius) {
    // 発射口はボスの足元位置のXZ、高さはkBossLaserHeight。モデルはローカル+Z方向に長さ1
    bossLaser->SetTranslate({ bossPos.x, kBossLaserHeight, bossPos.z });
    bossLaser->SetRotate({ 0.0f, yaw, 0.0f });
    bossLaser->SetScale({ radius, radius, kBossLaserLength });
}

void GameScene::StopBossLaser() {
    bossLaserVisible = false;
    bossLaserHasHit = false;
}

float GameScene::BossAttackCooldown() const {
    return (bossPhase == BossPhase::kP3) ? kBossAttackCooldown * kBossPhase3CooldownScale : kBossAttackCooldown;
}

bool GameScene::TryPickBossTeleportDest() {
    std::random_device seedGen;
    std::mt19937 randomEngine(seedGen());
    std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * 3.14159265f);
    std::uniform_real_distribution<float> distRadius(kBossTeleportMinDistance, kBossTeleportMaxDistance);
    const float kGroundOffset = 0.05f;
    for (int i = 0; i < kBossTeleportMaxTries; ++i) {
        float angle = distAngle(randomEngine);
        float radius = distRadius(randomEngine);
        float x = std::clamp(playerPos.x + std::sin(angle) * radius, -kBossTeleportArenaLimit, kBossTeleportArenaLimit);
        float z = std::clamp(playerPos.z + std::cos(angle) * radius, -kBossTeleportArenaLimit, kBossTeleportArenaLimit);
        if (!CheckCollision({ x, kGroundOffset, z }, nullptr, kBossCollisionRadius)) {
            bossTeleportDest = { x, 0.0f, z }; // bossPosと同じく足元基準（床y=0）
            return true;
        }
    }
    return false;
}

void GameScene::StopBossTeleport() {
    bool teleporting = bossAttackState == BossAttackState::kTeleportOut || bossAttackState == BossAttackState::kTeleportIn;
    // 何もしていないときは他の演出（開始演出のディゾルブ等）を邪魔しないよう何も触らない
    if (!teleporting && !bossTeleportMarkerVisible) return;
    bossTeleportMarkerVisible = false;
    SetBossDissolve(0.0f, 0.0f, 0.0f);
    screenEffect->SetTransitionGlitch(0.0f, 0.0f);
    if (teleporting) {
        bossAttackState = BossAttackState::kChasing; // 打ち切り時の安全策（呼び出し側が上書きする場合もある）
    }
}

void GameScene::StopBossShockwave() {
    shockwaveActive = false;
    shockwaveHasHit = false;
    shockwaveRadius = 0.0f;
}

void GameScene::RestartGame() {
    playerHP = kPlayerMaxHP;
    bossHP = kBossMaxHP;
    gameOver = false;
    playerWon = false;
    gameOverFadeTimer = 0.0f;
    gameOverOverlay->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });

    // プレイヤーをスポーン地点へ戻す（Initialize()の初期値と同じ）
    playerPos = { 0.0f, 0.0f, -8.0f };
    velocity = { 0.0f, 0.0f, 0.0f };
    isOnGround = false;
    Transform& cameraTransform = camera->GetTransform();
    cameraTransform.translate = { playerPos.x, playerPos.y + eyeHeight, playerPos.z };
    cameraTransform.rotate = { 0.0f, 0.0f, 0.0f };

    knockbackVelocity = { 0.0f, 0.0f, 0.0f };
    knockbackVelocityTimer = 0.0f;
    portalExitVelocity = { 0.0f, 0.0f, 0.0f };
    portalExitVelocityTimer = 0.0f;
    cameraShakeTimer = 0.0f;
    weaponRecoilTimer = 0.0f;
    weaponPumpTimer = 0.0f;
    fireCooldownTimer = 0.0f;

    // ボスをスポーン地点・追跡状態へ戻す
    bossPos = ArenaLayout::kBossInitialPos;
    bossAttackState = BossAttackState::kChasing;
    bossAttackTimer = 0.0f;
    bossAvoiding = false;
    bossAvoidDir = {};
    bossStuckTimer = 0.0f;
    bossGrenadeBurstRemaining = 0;
    bossGrenadeBurstTimer = 0.0f;
    bossDashCurrentSpeed = 0.0f;
    bossWarningEmissive = { 0.0f, 0.0f, 0.0f };
    bossOpenAmount = 0.0f;
    bossHitFlashTimer = 0.0f;
    bossPhase = BossPhase::kP1;
    phaseTextTimer = 0.0f;
    bossLaserCenterYaw = 0.0f;
    StopBossLaser();
    StopBossTeleport();
    StopBossShockwave();
    bossTeleportDest = { 0.0f, 0.0f, 0.0f };
    bossPhase3PulseTimer = 0.0f;
    // ボスの見た目の位置・向きも戻す。READY中はAIが止まっていてbossのSetTranslateが呼ばれないため、
    // ここで戻さないと、FIGHTまで前の試合で倒れた位置・向きのまま表示されてしまう
    boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });
    boss->SetRotate({ 0.0f, 0.0f, 0.0f }); // Initialize時と同じ向き
    boss->SetScale(ArenaLayout::kBossScale);
    SetBossDissolve(0.0f, 0.0f, 0.0f);

    // 画面効果（被ダメージのビネット・ワープのグレースケール等）の残りを消す
    screenEffect->ResetTransientEffects();

    // 前の試合の弾痕を消す（未使用分と同じく画面外へ隠す）
    for (auto& decal : bulletDecals) {
        decal->SetTranslate({ 0.0f, -1000.0f, 0.0f });
    }
    nextDecalIndex = 0;

    // 前の試合のパーティクル（マズルフラッシュ・火花・粉塵）を消す。
    // 決着後は Update が止まるため、消さないと READY の間も止まったまま残る
    particleManagerLine->Clear();
    particleManagerPoint->Clear();
    particleManagerTriangle->Clear();

    // グレネードは全て非アクティブ化して隠す
    for (auto& grenade : grenades) {
        grenade.active = false;
        grenade.object->SetTranslate({ 0.0f, -1000.0f, 0.0f });
    }

    // 的・ヒットフラッシュ・ポータル・武器モードもクリアして、まっさらな状態から再プレイできるようにする
    for (auto& enemy : enemies) {
        enemy.isActive = true;
        enemy.respawnTimer = 0.0f;
    }
    for (auto& flash : hitFlashes) {
        flash.object->SetColor(flash.originalColor);
    }
    hitFlashes.clear();

    weaponMode = WeaponMode::kRifle;
    portalA.isPlaced = false;
    portalB.isPlaced = false;
    if (portalA.visual) portalA.visual->SetScale({ 0.0f, 0.0f, 0.0f });
    if (portalB.visual) portalB.visual->SetScale({ 0.0f, 0.0f, 0.0f });
    portalRejectTimer = 0.0f;

    // リトライ時は短縮版の開始演出を流す（暗転中に呼ばれるため、明転とともにREADYが見える）
    StartIntro(false);
}

void GameScene::StartIntro(bool fullVersion) {
    introPhase = IntroPhase::kReady;
    introIsFull = fullVersion;
    introElapsed = 0.0f;
    introSavedCameraRotate = camera->GetTransform().rotate;
    SetIntroText("READY", kIntroReadyColor);
    UpdateIntroVisual(); // 初回フレームの描画前に黒帯・HPバーの初期状態を確定させる
}

void GameScene::UpdateIntroVisual() {
    const float screenWidth = (float)WinApp::KclientWidth;
    const float screenHeight = (float)WinApp::KclientHeight;

    // 文字：READYはフル版だと黒帯・チャージの後に出す。FIGHTは時間とともに消えていく
    float textAlpha = 1.0f;
    if (introPhase == IntroPhase::kReady) {
        textAlpha = (!introIsFull || introElapsed >= kIntroFullTextAppearTime) ? 1.0f : 0.0f;
    } else if (introPhase == IntroPhase::kFight) {
        textAlpha = 1.0f - introElapsed / kIntroFightDuration;
    }
    const Vector4& textColor = (introPhase == IntroPhase::kFight) ? kIntroFightColor : kIntroReadyColor;
    introText.SetColor({ textColor.x, textColor.y, textColor.z, textAlpha });
    introText.Update();

    // グリッチ（シーン遷移用のグリッチを流用）。明転中はSceneTransition側が毎フレーム書き込むため、
    // 明転が終わった後の時間帯（FIGHT、またはフル版の実体化開始以降）だけ書き込む
    if (introPhase == IntroPhase::kFight) {
        float glitch = kIntroFightGlitch * (1.0f - Easing::OutCubic(Easing::Clamp01(introElapsed / kIntroFightGlitchDuration)));
        screenEffect->SetTransitionGlitch(glitch, introElapsed);
    } else if (introIsFull && introElapsed >= kIntroMaterializeStart) {
        float t = std::clamp((introElapsed - kIntroMaterializeStart) / kIntroMaterializeDuration, 0.0f, 1.0f);
        screenEffect->SetTransitionGlitch(kIntroMaterializeGlitch * (1.0f - t), introElapsed);
    }

    if (!introIsFull) {
        bossHpBarIntroScale = 1.0f;
        return;
    }

    // ボスの実体化（READY中のみ）：形は最初から完成形のまま、描画範囲の上端を足元から頭上へ走らせて
    // 下から姿が転送されてくるように見せる。前半は数フレームおきに消してちらつかせる
    if (introPhase == IntroPhase::kReady) {
        float materializeT = std::clamp((introElapsed - kIntroMaterializeStart) / kIntroMaterializeDuration, 0.0f, 1.0f);
        float bossHeight = kBossHalfHeight * 2.0f;
        // 発光帯が頭頂を抜け切るまで走らせる
        float revealHeight = (bossHeight + kIntroMaterializeEdgeWidth) * Easing::OutCubic(Easing::Clamp01(materializeT));
        // ブロック欠けは境界が頭頂に近づくにつれ弱め、頭頂に届いた（全体が見えた）時点で0にする
        float blockStrength = std::clamp((1.0f - revealHeight / bossHeight) / kIntroMaterializeBlockFadeRatio, 0.0f, 1.0f);
        int frame = static_cast<int>(introElapsed * 60.0f);
        if (materializeT > 0.0f && materializeT < kIntroMaterializeFlickerEnd && frame % 3 == 0) {
            revealHeight = 0.0f;
        }
        boss->SetScale(ArenaLayout::kBossScale);
        boss->SetTranslate({ bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z });
        // 完了後は演出を切る（edgeWidth=0で無効）
        SetBossDissolve(bossPos.y + revealHeight, materializeT < 1.0f ? kIntroMaterializeEdgeWidth : 0.0f, introElapsed, blockStrength);
    } else {
        // 短縮版はREADY(1.6s)と実体化完了(0.6+1.0s)がほぼ同時で、完了前にFIGHTへ移ることがあるため、READY外では必ず演出を切る
        SetBossDissolve(0.0f, 0.0f, 0.0f);
    }

    // 黒帯：READY開始時に上下から入り、READY終盤で引き切る（FIGHT＝操作解禁の時点では帯なし）
    float barRatio = 0.0f;
    if (introPhase == IntroPhase::kReady) {
        float retractStart = kIntroFullReadyDuration - kIntroBarRetractDuration;
        float slideIn = Easing::OutCubic(Easing::Clamp01(introElapsed / kIntroBarSlideDuration));
        float retract = Easing::InCubic(Easing::Clamp01((introElapsed - retractStart) / kIntroBarRetractDuration));
        barRatio = slideIn * (1.0f - retract);
    }
    float barHeight = kIntroBarHeight * barRatio;
    introBarTop->SetSize({ screenWidth, barHeight });
    introBarBottom->SetPosition({ 0.0f, screenHeight - barHeight });
    introBarBottom->SetSize({ screenWidth, barHeight });
    introBarTop->Update();
    introBarBottom->Update();

    // ボスHPバー：READY中に0から満タンまでチャージする（FIGHT以降は常に満タン表示）
    bossHpBarIntroScale = (introPhase == IntroPhase::kReady)
        ? Easing::OutCubic(Easing::Clamp01((introElapsed - kIntroHpChargeStart) / (kIntroHpChargeEnd - kIntroHpChargeStart)))
        : 1.0f;
}

void GameScene::UpdateIntroCamera() {
    if (!introIsFull) { return; }
    Transform& cameraTransform = camera->GetTransform();
    const Vector3 playerEye = { playerPos.x, playerPos.y + eyeHeight, playerPos.z };

    // 演出用カメラ：READY全体をかけてStart→Endへ少しずつ寄り、常にボスの注視点を向く
    float pushT = introElapsed / kIntroFullReadyDuration;
    Vector3 cinePos = {
        kIntroCameraStartPos.x + (kIntroCameraEndPos.x - kIntroCameraStartPos.x) * pushT,
        kIntroCameraStartPos.y + (kIntroCameraEndPos.y - kIntroCameraStartPos.y) * pushT,
        kIntroCameraStartPos.z + (kIntroCameraEndPos.z - kIntroCameraStartPos.z) * pushT,
    };
    Vector3 toTarget = { bossPos.x - cinePos.x, bossPos.y + kIntroCameraLookHeight - cinePos.y, bossPos.z - cinePos.z };
    float cineYaw = std::atan2(toTarget.x, toTarget.z); // forward = (sin(yaw), 0, cos(yaw)) の規約
    float cinePitch = std::atan2(-toTarget.y, std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z)); // rotate.xは正で下向き

    // 黒帯が引く間（READY終盤）にプレイヤー視点へ補間して戻す。FIGHTに入った後は完全にプレイヤー視点
    float retractStart = kIntroFullReadyDuration - kIntroBarRetractDuration;
    float back = (introPhase == IntroPhase::kReady)
        ? Easing::InOutCubic(Easing::Clamp01((introElapsed - retractStart) / kIntroBarRetractDuration))
        : 1.0f;
    cameraTransform.translate = {
        cinePos.x + (playerEye.x - cinePos.x) * back,
        cinePos.y + (playerEye.y - cinePos.y) * back,
        cinePos.z + (playerEye.z - cinePos.z) * back,
    };
    cameraTransform.rotate = {
        cinePitch + (introSavedCameraRotate.x - cinePitch) * back,
        cineYaw + (introSavedCameraRotate.y - cineYaw) * back,
        introSavedCameraRotate.z,
    };
}

void GameScene::SetIntroText(const std::string& text, const Vector4& color) {
    introText.SetText(text);
    introText.SetColor(color);
    introText.SetPosition({ ((float)WinApp::KclientWidth - introText.GetWidth()) / 2.0f, kIntroTextPositionY });
}

void GameScene::FireGrenade() {
    // 非アクティブなグレネードを1つ探して発射する
    for (auto& grenade : grenades) {
        if (grenade.active) continue;

        Vector3 launchPos = { bossPos.x, bossPos.y + kBossHalfHeight, bossPos.z }; // 胸の高さから発射

        // 着弾狙い位置：プレイヤーそのものではなく、周囲kGrenadeAimSpread以内にランダムでずらす。
        // これにより毎回同じ場所に落ちず、3連射で着弾点が散らばり、プレイヤーにも回避の余地が生まれる
        std::random_device seed_gen;
        std::mt19937 randomEngine(seed_gen());
        std::uniform_real_distribution<float> distAngle(0.0f, 6.28318f);
        std::uniform_real_distribution<float> distRadius(0.0f, kGrenadeAimSpread);
        float angle = distAngle(randomEngine);
        float spreadRadius = distRadius(randomEngine);
        Vector3 targetPos = {
            playerPos.x + std::cos(angle) * spreadRadius,
            launchPos.y,
            playerPos.z + std::sin(angle) * spreadRadius,
        };

        Vector3 toTarget = { targetPos.x - launchPos.x, 0.0f, targetPos.z - launchPos.z };
        float horizontalDistance = Calculation::Length(toTarget);
        Vector3 horizontalDir = (horizontalDistance > 0.0001f)
            ? Calculation::Normalize(toTarget)
            : Vector3{ 0.0f, 0.0f, 1.0f };

        // 山なり軌道：頂点の高さ(kGrenadeArcHeight)から逆算して上方向の初速を求め、
        // 頂点まで昇って同じ高さまで落ちてくる時間(flightFrames)で水平距離を割ることで、
        // 距離によらず「同じ高さまで放物線を描いて狙った位置の近くに着地する」速度を算出する
        float launchUpSpeed = std::sqrt(2.0f * -kGrenadeGravity * kGrenadeArcHeight);
        float flightFrames = (2.0f * launchUpSpeed) / -kGrenadeGravity;
        float horizontalSpeed = std::clamp(horizontalDistance / flightFrames, kGrenadeMinSpeed, kGrenadeMaxSpeed);

        grenade.position = launchPos;
        grenade.velocity = { horizontalDir.x * horizontalSpeed, launchUpSpeed, horizontalDir.z * horizontalSpeed };
        grenade.active = true;
        grenade.object->SetTranslate(grenade.position);
        break;
    }
}

void GameScene::ExplodeGrenade(Grenade& grenade) {
    grenade.active = false;
    grenade.object->SetTranslate({ 0.0f, -1000.0f, 0.0f }); // 未使用分は画面外に隠す（弾痕デカールと同じ手法）

    Vector3 explosionPos = grenade.position;

    // プレイヤーが爆発範囲内なら被弾（水平距離のみで判定。垂直方向の細かい判定は仮実装のスコープ外）
    Vector3 toPlayer = { playerPos.x - explosionPos.x, 0.0f, playerPos.z - explosionPos.z };
    if (Calculation::Length(toPlayer) <= kGrenadeExplosionRadius) {
        TakeDamage(kGrenadeDamage); // 内部でカメラシェイク・被弾ヴィネットも行われる
    }

    // 見た目：爆発点から全方位に広がる火花バースト（FireShotの着弾演出と同じEmit()の使い方を流用し、方向だけ全方位ランダムにする）
    std::random_device seed_gen;
    std::mt19937 randomEngine(seed_gen());
    std::uniform_real_distribution<float> distTheta(0.0f, 6.28318f);
    std::uniform_real_distribution<float> distCosPhi(-1.0f, 1.0f); // 球面上に均一分布させるための余弦
    std::uniform_real_distribution<float> distSpeed(2.0f, 6.0f);
    std::uniform_real_distribution<float> distLife(0.3f, 0.6f);

    auto sphereDir = [&]() {
        float theta = distTheta(randomEngine);
        float cosPhi = distCosPhi(randomEngine);
        float sinPhi = std::sqrt((std::max)(0.0f, 1.0f - cosPhi * cosPhi));
        return Vector3{ sinPhi * std::cos(theta), cosPhi, sinPhi * std::sin(theta) };
    };

    for (int i = 0; i < 30; ++i) {
        Vector3 dir = sphereDir();
        float speed = distSpeed(randomEngine);

        ParticleEmitParams params;
        params.position = explosionPos;
        params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
        params.lifeTime = distLife(randomEngine);

        params.useColorGradient = true;
        params.startColor = { 1.0f, 0.7f, 0.2f, 1.0f };
        params.endColor = { 1.0f, 0.1f, 0.0f, 0.0f };

        params.gravity = { 0.0f, -0.015f, 0.0f };
        params.drag = 0.1f;

        particleManagerPoint->Emit(params);
    }

    std::uniform_real_distribution<float> distSize(0.1f, 0.3f);
    for (int i = 0; i < 12; ++i) {
        Vector3 dir = sphereDir();
        float speed = distSpeed(randomEngine) * 0.6f;
        float size = distSize(randomEngine);

        ParticleEmitParams params;
        params.position = explosionPos;
        params.velocity = { dir.x * speed, dir.y * speed, dir.z * speed };
        params.lifeTime = distLife(randomEngine) * 1.3f;

        params.useColorGradient = true;
        params.startColor = { 1.0f, 0.5f, 0.15f, 0.8f };
        params.endColor = { 0.4f, 0.1f, 0.05f, 0.0f };

        params.gravity = { 0.0f, 0.0f, 0.0f };
        params.drag = 0.2f;

        params.startScale = { size, size, size };
        params.endScale = { size * 1.6f, size * 1.6f, size * 1.6f };

        particleManagerTriangle->Emit(params);
    }
}

void GameScene::FireShot() {
    // 命中判定に関わらず、発砲した時点でリコイル演出とカメラシェイクを開始する
    weaponRecoilTimer = kRecoilDuration;
    weaponPumpTimer = kPumpDuration; // 先台のポンプ動作もここで開始
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
            bool isBossHit = (closestObject == boss.get());
            bool isCoreHit = false;
            if (isBossHit) {
                // 距離で線形減衰するダメージ（近距離ほど高ダメージ＝ショットガン的な仕様）
                float t = std::clamp((closestHit.distance - kBossDamageFalloffStart) / (kBossDamageFalloffEnd - kBossDamageFalloffStart), 0.0f, 1.0f);
                float damage = kBossDamageMax + (kBossDamageMin - kBossDamageMax) * t;

                // コア命中判定：bossの箱はコアを内側に含むので、bossが最前面のときだけコアのAABBを追加で調べる。
                // くさびがある程度開いている（P2以降、またはP1で開き具合が十分）ときだけコアが露出している
                if (bossPhase != BossPhase::kP1 || bossOpenAmount >= kBossCoreExposedOpenAmount) {
                    Vector3 coreCenter = { bossPos.x, bossPos.y + kBossHalfHeight + kBossCoreCenterOffsetY, bossPos.z };
                    AABB coreAABB;
                    coreAABB.min = { coreCenter.x - kBossCoreHalfSize, coreCenter.y - kBossCoreHalfSize, coreCenter.z - kBossCoreHalfSize };
                    coreAABB.max = { coreCenter.x + kBossCoreHalfSize, coreCenter.y + kBossCoreHalfSize, coreCenter.z + kBossCoreHalfSize };
                    if (Calculation::TestRayAABB(ray, coreAABB)) {
                        damage *= kBossCoreDamageMultiplier;
                        isCoreHit = true;
                    }
                }
                DamageBoss(damage);
            }

            // ボスは移動するため、ワールド座標に固定される弾痕デカールを出すと空中に取り残されて見える。
            // 壁・床など静止オブジェクトのみ弾痕を残す（ヒットフラッシュ・カメラシェイクはボスにも残す）
            if (!isBossHit) {
                SpawnBulletDecal(closestHit.hitPoint, normal, closestAABB);
            }

            if (isBossHit) {
                // bossは描画しない芯なので色を変えても見えない。代わりにくさびの赤発光タイマーを使う
                bossHitFlashTimer = kBossHitFlashDuration;
            } else {
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
            }

            // 3. カメラシェイク：発砲時より少し大きく揺らして命中の「重み」を出す
            TriggerCameraShake(kHitShakeDuration, isCoreHit ? kBossCoreHitShakeStrength : kHitShakeStrength);

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

    // 先台のポンプ動作（リコイルと同じ固定ステップ）。発砲0.1秒後から引き、0.25秒で引き切って0.4秒で戻る
    if (weaponPumpTimer > 0.0f) {
        weaponPumpTimer -= 1.0f / 60.0f;
        if (weaponPumpTimer < 0.0f) weaponPumpTimer = 0.0f;
    }
    float pumpOffset = 0.0f;
    if (weaponPumpTimer > 0.0f) {
        float e = kPumpDuration - weaponPumpTimer;
        if (e < kPumpPullStart) {
            pumpOffset = 0.0f;
        } else if (e < kPumpPullEnd) {
            float t = (e - kPumpPullStart) / (kPumpPullEnd - kPumpPullStart);
            pumpOffset = kPumpStroke * Easing::OutCubic(t);
        } else {
            float t = (e - kPumpPullEnd) / (kPumpDuration - kPumpPullEnd);
            pumpOffset = kPumpStroke * (1.0f - Easing::InOutCubic(t));
        }
    }
    // 先台は本体と同じ原点で、銃の後ろ方向（ローカル-Z）へpumpOffsetだけ引く
    Vector3 pumpLocal = bodyLocal;
    pumpLocal.z -= pumpOffset;

    Vector3 bodyWorld = Calculation::Transform(bodyLocal, cameraWorldMatrix);
    Vector3 pumpWorld = Calculation::Transform(pumpLocal, cameraWorldMatrix);

    weaponBody->SetTranslate(bodyWorld);
    weaponBody->SetRotate(cameraTransform.rotate);

    weaponPump->SetTranslate(pumpWorld);
    weaponPump->SetRotate(cameraTransform.rotate);

    // ポータルガンは本体と同じ位置（先台の動きは無し）
    weaponPortalGun->SetTranslate(bodyWorld);
    weaponPortalGun->SetRotate(cameraTransform.rotate);

    weaponBody->Update();
    weaponPump->Update();
    weaponPortalGun->Update();
}

void GameScene::UpdateWeaponHud() {
    // 後ろのパネル（位置・色は固定。Initializeで設定済み）
    for (size_t i = 0; i < weaponPanels.size(); ++i) {
        weaponPanels[i]->Update();
        weaponPanelLines[i]->Update();
    }

    // 今の武器を手前（カラー・大きく）、もう一方を後ろ（グレースケール・小さく・半透明）に置く
    bool isPortal = (weaponMode == WeaponMode::kPortalGun);
    Sprite* back = weaponIcons[isPortal ? kIconShotgunGray : kIconPortalGunGray].get();
    back->SetAnchorPoint({ 1.0f, 1.0f });
    back->SetPosition(kWeaponIconBackPos);
    back->SetSize(kWeaponIconBackSize);
    back->SetTexLeftTop({ 0.0f, 0.0f });
    back->SetTexSize(kWeaponIconTexSize);
    back->SetColor(kWeaponIconBackColor);
    back->Update();

    if (isPortal) {
        // ポータルガンにはクールタイムが無いので、カラーをそのまま出す
        Sprite* front = weaponIcons[kIconPortalGun].get();
        front->SetPosition(kWeaponIconFrontPos);
        front->SetSize(kWeaponIconFrontSize);
        front->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        front->Update();
    } else {
        // ショットガン：下にグレースケールを敷き、クールタイムの明けた割合だけカラーを左から重ねる（ゲージ）
        float ready = 1.0f - std::clamp(fireCooldownTimer / kFireCooldownDuration, 0.0f, 1.0f);

        Sprite* base = weaponIcons[kIconShotgunGray].get();
        base->SetAnchorPoint({ 1.0f, 1.0f });
        base->SetPosition(kWeaponIconFrontPos);
        base->SetSize(kWeaponIconFrontSize);
        base->SetTexLeftTop({ 0.0f, 0.0f });
        base->SetTexSize(kWeaponIconTexSize);
        base->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        base->Update();

        // 左端を基準（アンカー(0,1)）にして、表示幅と画像の切り抜き幅を同じ割合で縮める
        Sprite* fill = weaponIcons[kIconShotgun].get();
        fill->SetAnchorPoint({ 0.0f, 1.0f });
        fill->SetPosition({ kWeaponIconFrontPos.x - kWeaponIconFrontSize.x, kWeaponIconFrontPos.y });
        fill->SetSize({ kWeaponIconFrontSize.x * ready, kWeaponIconFrontSize.y });
        fill->SetTexLeftTop({ 0.0f, 0.0f });
        fill->SetTexSize({ kWeaponIconTexSize.x * ready, kWeaponIconTexSize.y });
        fill->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        fill->Update();
    }

    // キー番号（1＝ショットガン、2＝ポータルガン）
    weaponSlotTextFront.SetText(isPortal ? "2" : "1");
    weaponSlotTextBack.SetText(isPortal ? "1" : "2");
    weaponSlotTextFront.Update();
    weaponSlotTextBack.Update();
}

void GameScene::SetBossDissolve(float height, float edgeWidth, float time, float blockStrength) {
    boss->SetDissolve(height, edgeWidth, time, blockStrength);
    for (auto& part : bossParts) {
        part->SetDissolve(height, edgeWidth, time, blockStrength);
    }
}

void GameScene::UpdateBossParts() {
    // bossは当たり判定の芯。位置・回転・scaleを5パーツへ写し、くさびだけ開閉オフセットを足す
    const Transform& core = boss->GetTransform();
    const float openT = Easing::OutCubic(Easing::Clamp01(bossOpenAmount));
    const float cosYaw = std::cos(core.rotate.y);
    const float sinYaw = std::sin(core.rotate.y);

    // 被弾フラッシュ（赤）を警告色より優先する。時間で減衰
    Vector3 emissive = bossWarningEmissive;
    float intensity = kBossWarningEmissiveIntensity;
    // P3は警告がないとき、赤で脈打つのを基本色にする
    if (bossPhase == BossPhase::kP3 && emissive.x == 0.0f && emissive.y == 0.0f && emissive.z == 0.0f) {
        emissive = kBossPhase3Emissive;
        intensity = kBossPhase3EmissiveBase + kBossPhase3EmissivePulse * std::sin(bossPhase3PulseTimer * kBossPhase3PulseSpeed);
    }
    if (bossHitFlashTimer > 0.0f) {
        emissive = { 1.0f, 0.1f, 0.1f };
        intensity = kBossHitFlashIntensity * (bossHitFlashTimer / kBossHitFlashDuration);
    }

    for (int i = 0; i < kBossPartCount; ++i) {
        Vector3 offset = { 0.0f, 0.0f, 0.0f };
        const float dist = kBossWedgeOpenDistance * openT;
        switch (i) {
        case kBossWedgeTop:    offset.y = dist; break;
        case kBossWedgeBottom: offset.y = -kBossWedgeOpenDistanceBottom * openT; break;
        case kBossWedgeLeft:   offset.x = -dist * cosYaw; offset.z = dist * sinYaw; break;  // ローカル(-1,0,0)をyaw回転
        case kBossWedgeRight:  offset.x = dist * cosYaw;  offset.z = -dist * sinYaw; break; // ローカル(+1,0,0)をyaw回転
        default: break; // コアは動かさない
        }

        bossParts[i]->SetScale(core.scale);
        bossParts[i]->SetRotate(core.rotate);
        bossParts[i]->SetTranslate({ core.translate.x + offset.x, core.translate.y + offset.y, core.translate.z + offset.z });
        if (i != kBossCore) {
            bossParts[i]->SetEmissive(emissive, intensity);
        }
        bossParts[i]->Update(); // 呼び忘れると単位行列のまま巨大に描画される
    }
}

void GameScene::DrawWeaponViewModel() {
    Framework* framework = Framework::GetInstance();
    auto commandList = framework->GetDxCommon()->GetCommandList();

    // 直前にParticleCommonのPSOがバインドされているため、Object3d用に再バインドする
    framework->GetObject3dCommon()->PreDraw();

    // 深度バッファのみをクリアし、既に描画済みの壁などにめり込んで隠れるのを防ぐ
    commandList->ClearDepthStencilView(
        framework->GetDxCommon()->GetMainDsvHandle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // 今のモードの武器だけを描く（1キー＝ショットガン、2キー＝ポータルガン）
    if (weaponMode == WeaponMode::kPortalGun) {
        weaponPortalGun->Draw();
    } else {
        weaponBody->Draw();
        weaponPump->Draw();
    }
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
    Vector3 candidate = {
        hit.hitPoint.x + normal.x * 0.02f,
        hit.hitPoint.y + normal.y * 0.02f,
        hit.hitPoint.z + normal.z * 0.02f,
    };

    // もう一方のポータルが置いてあるときは、離れすぎた場所には置かない（レティクルの色で知らせる）
    const Portal& other = (&portal == &portalA) ? portalB : portalA;
    if (other.isPlaced) {
        Vector3 diff = { candidate.x - other.position.x, candidate.y - other.position.y, candidate.z - other.position.z };
        if (Calculation::Length(diff) > kPortalMaxPairDistance) {
            portalRejectTimer = kPortalRejectFlashDuration;
            return;
        }
    }

    portal.position = candidate;
    portal.normal = normal;
    portal.orientation = Calculation::DirectionToDirection({ 0.0f, 0.0f, 1.0f }, normal);
    portal.isPlaced = true;

    if (!portal.visual) {
        Framework* framework = Framework::GetInstance();
        portal.visual = std::make_unique<Object3d>();
        portal.visual->Initialize(framework->GetObject3dCommon());
        portal.visual->SetModel((&portal == &portalA) ? portalModelA : portalModelB);
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

    // portalAは青、portalBはオレンジの色はモデル側に付いているので、基本色は白にする
    portal.visual->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
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
    screenEffect->TriggerGrayscaleFlash();
}
