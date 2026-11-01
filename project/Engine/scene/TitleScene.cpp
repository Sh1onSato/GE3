#include "TitleScene.h"
#include "GameSettings.h"
#include "SceneManager.h"
#include "Framework.h"
#include "WinApp.h"
#include "Input.h"
#include "CameraManager.h"
#include "Calculation.h"
#include <algorithm>
#include <cmath>
#include <vector>

void TitleScene::Initialize() {
    Framework* framework = Framework::GetInstance();

    background = std::make_unique<Sprite>();
    background->Initialize(framework->GetSpriteCommon(), "white");
    background->SetPosition({ 0.0f, 0.0f });
    background->SetSize({ (float)WinApp::KclientWidth, (float)WinApp::KclientHeight });
    background->SetColor({ kDimColor.x, kDimColor.y, kDimColor.z, kDimAlphaMain });

    InitializeBackground3d();
    InitializeSettingsMenu();
    InitializeTexts();

    state = State::Main;
    blinkTimer = 0.0f;
    selectedIndex = kMenuStart;

    // タイトルではマウスカーソルを使えるようにしておく（GameSceneが開始時に隠す）
    framework->GetWinApp()->ShowCursor(true);
    framework->GetWinApp()->SetClipCursor(false);
}

void TitleScene::InitializeBackground3d() {
    Framework* framework = Framework::GetInstance();

    // タイトル専用カメラを登録してアクティブにする。
    // Object3d/SkyboxはGetActiveCamera()を参照するため、GameSceneの"MainCamera"が残っていても確実にこちらを使わせる
    CameraManager* cameraManager = CameraManager::GetInstance();
    cameraManager->AddCamera(kCameraName, std::make_unique<Camera>());
    cameraManager->SetActiveCamera(kCameraName);
    camera = cameraManager->GetCamera(kCameraName);

    cubeModel = std::make_unique<Model>();
    cubeModel->Initialize(framework->GetDxCommon(), "Resources/cube", "cube.obj");

    skybox = std::make_unique<Skybox>();
    skybox->Initialize(framework->GetSkyboxCommon(), "Resources/skybox/sky_cubemap.dds");
    skybox->SetModel(cubeModel.get());

    // GameSceneと同じ経路（オフスクリーン→PostProcess::Draw）で描き、見た目を揃える
    postProcess = framework->GetPostProcess();
    postProcess->ResetTransientEffects();

    InitializeArenaObjects();

    backgroundTimer = 0.0f;
    UpdateBackground3d();
}

void TitleScene::InitializeArenaObjects() {
    Framework* framework = Framework::GetInstance();

    floor = std::make_unique<Object3d>();
    floor->Initialize(framework->GetObject3dCommon());
    floor->SetModel(cubeModel.get());
    floor->SetScale(ArenaLayout::kFloorScale);
    floor->SetTranslate(ArenaLayout::kFloorTranslate);
    floor->SetColor(ArenaLayout::kFloorColor);

    for (const auto& data : ArenaLayout::kWallDatas) {
        auto wall = std::make_unique<Object3d>();
        wall->Initialize(framework->GetObject3dCommon());
        wall->SetModel(cubeModel.get());
        wall->SetTranslate(data.pos);
        wall->SetScale(data.scale);
        walls.push_back(std::move(wall));
    }

    boss = std::make_unique<Object3d>();
    boss->Initialize(framework->GetObject3dCommon());
    boss->SetModel(cubeModel.get());
    boss->SetScale(ArenaLayout::kBossScale);
    boss->SetColor(ArenaLayout::kBossColor);
}

void TitleScene::InitializeSettingsMenu() {
    // マウス感度・目線の高さをGameSettings経由で調整する（GameScene::Initialize()で読み込まれ引き継がれる）
    std::vector<SettingsMenu::Item> items;

    SettingsMenu::Item sensitivityItem;
    sensitivityItem.color = { 0.3f, 0.7f, 1.0f, 1.0f };
    sensitivityItem.minValue = GameSettings::kMouseSensitivityMin;
    sensitivityItem.maxValue = GameSettings::kMouseSensitivityMax;
    sensitivityItem.step = 0.0005f;
    sensitivityItem.displayDecimals = 4;
    sensitivityItem.getValue = []() { return GameSettings::GetInstance()->GetMouseSensitivity(); };
    sensitivityItem.setValue = [](float v) { GameSettings::GetInstance()->SetMouseSensitivity(v); };
    items.push_back(sensitivityItem);

    SettingsMenu::Item eyeHeightItem;
    eyeHeightItem.color = { 0.3f, 1.0f, 0.5f, 1.0f };
    eyeHeightItem.minValue = GameSettings::kEyeHeightMin;
    eyeHeightItem.maxValue = GameSettings::kEyeHeightMax;
    eyeHeightItem.step = 0.1f;
    eyeHeightItem.displayDecimals = 1;
    eyeHeightItem.getValue = []() { return GameSettings::GetInstance()->GetEyeHeight(); };
    eyeHeightItem.setValue = [](float v) { GameSettings::GetInstance()->SetEyeHeight(v); };
    items.push_back(eyeHeightItem);

    settingsMenu.Initialize(Framework::GetInstance()->GetSpriteCommon(), items);
}

void TitleScene::SetupText(BitmapText& text, size_t maxChars, const std::string& str, float scale, const Vector4& color) {
    text.Initialize(Framework::GetInstance()->GetSpriteCommon(), maxChars);
    text.SetScale(scale);
    text.SetText(str);
    text.SetColor(color);
}

void TitleScene::SetCenteredPosition(BitmapText& text, float y) {
    text.SetPosition({ ((float)WinApp::KclientWidth - text.GetWidth()) / 2.0f, y });
}

void TitleScene::InitializeTexts() {
    // --- メイン画面 ---
    std::string title = kTitleText;
    SetupText(titleText, title.size(), title, kTitleScale, kTitleColor);
    SetCenteredPosition(titleText, kTitlePositionY);

    for (int i = 0; i < kMenuCount; ++i) {
        std::string label = kMenuLabels[i];
        SetupText(menuTexts[i], label.size(), label, kMenuScale, kUnselectedColor);
        SetCenteredPosition(menuTexts[i], kMenuPositionY + kMenuSpacing * static_cast<float>(i));
    }

    SetupText(cursorText, 1, ">", kMenuScale, kSelectedColor);

    const std::string guide = "W S SELECT   ENTER OK";
    SetupText(guideText, guide.size(), guide, kGuideScale, kUnselectedColor);
    SetCenteredPosition(guideText, (float)WinApp::KclientHeight - kGuideBottomMargin);

    // --- 設定画面 ---
    const std::string heading = "SETTINGS";
    SetupText(settingsHeading, heading.size(), heading, kHeadingScale, kTitleColor);
    SetCenteredPosition(settingsHeading, kHeadingPositionY);

    // 行ラベルはバー(SettingsMenuの行座標)の左に右揃えで並べる
    for (size_t i = 0; i < settingsLabels.size(); ++i) {
        std::string label = kSettingsLabelTexts[i];
        SetupText(settingsLabels[i], label.size(), label, 2.0f, kTitleColor);
        Vector2 rowPos = SettingsMenu::GetRowPosition(i);
        settingsLabels[i].SetPosition({ rowPos.x - SettingsMenu::kLabelRightMargin - settingsLabels[i].GetWidth(), rowPos.y });
    }

    const std::string hint = "ESC BACK";
    SetupText(settingsHint, hint.size(), hint, kGuideScale, kUnselectedColor);
    SetCenteredPosition(settingsHint, kSettingsHintPositionY);
}

void TitleScene::Update() {
    blinkTimer += kDeltaTime;

    switch (state) {
    case State::Main:
        UpdateMain();
        break;
    case State::Settings:
        UpdateSettings();
        break;
    }

    UpdateBackground3d();
    UpdateDimOverlay();
    UpdateMenuColors();
    UpdateTexts();
}

void TitleScene::UpdateBackground3d() {
    backgroundTimer += kDeltaTime;

    // ボスの足元位置（初期位置）と胴体中心。浮遊はボス本体の描画位置にだけ加え、注視点は揺らさない
    const Vector3 bossBase = ArenaLayout::kBossInitialPos;
    const float bossCenterY = bossBase.y + ArenaLayout::kBossHalfHeight;
    const Vector3 lookTarget = { bossBase.x, bossCenterY + kLookAtHeightOffset, bossBase.z };
    const float floatOffset = std::sin(backgroundTimer * kBossFloatSpeed) * kBossFloatAmplitude;
    boss->SetTranslate({ bossBase.x, bossCenterY + floatOffset, bossBase.z });

    // カメラはアリーナ中央を中心に周回し、常にボスの胴体を向く
    const float angle = backgroundTimer * kOrbitAngularSpeed;
    const Vector3 cameraPos = {
        kOrbitCenter.x + std::cos(angle) * kOrbitRadius,
        kOrbitCenter.y + kOrbitHeight,
        kOrbitCenter.z + std::sin(angle) * kOrbitRadius,
    };
    camera->SetTranslate(cameraPos);
    camera->SetRotate(Calculation::MakeLookAtRotation(cameraPos, lookTarget));
    // Object3d::Update()が参照するビュー行列を先に確定させる
    camera->Update();

    skybox->Update();
    floor->Update();
    for (auto& wall : walls) { wall->Update(); }
    boss->Update();
    postProcess->Update(false);
}

void TitleScene::UpdateDimOverlay() {
    const float alpha = (state == State::Settings) ? kDimAlphaSettings : kDimAlphaMain;
    background->SetColor({ kDimColor.x, kDimColor.y, kDimColor.z, alpha });
}

void TitleScene::UpdateMain() {
    Input* input = Framework::GetInstance()->GetInput();

    if (input->TriggerKey(DIK_UP) || input->TriggerKey(DIK_W)) {
        selectedIndex = (selectedIndex + kMenuCount - 1) % kMenuCount;
    }
    if (input->TriggerKey(DIK_DOWN) || input->TriggerKey(DIK_S)) {
        selectedIndex = (selectedIndex + 1) % kMenuCount;
    }
    // ESCは「終了したい」操作とみなし、カーソルをQUITへ移す（即終了はせず、Enterで確定させる）
    if (input->TriggerKey(DIK_ESCAPE)) {
        selectedIndex = kMenuQuit;
    }

    if (input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE)) {
        switch (selectedIndex) {
        case kMenuStart:
            // フェードはSceneManager側が担当（遷移中の再要求は無視される）
            sceneManager->RequestTransition("GAME");
            break;
        case kMenuSettings:
            state = State::Settings;
            break;
        case kMenuQuit:
            Framework::GetInstance()->RequestEnd();
            break;
        }
    }
}

void TitleScene::UpdateSettings() {
    Input* input = Framework::GetInstance()->GetInput();

    // SettingsMenuも↑↓を使うため、Settings中はメインメニューの入力を処理しない
    settingsMenu.Update(input, true);

    if (input->TriggerKey(DIK_ESCAPE)) {
        state = State::Main;
    }
}

void TitleScene::UpdateMenuColors() {
    // 選択中の項目はsin波でアルファを明滅、それ以外は灰色固定
    float pulse = 0.5f + 0.5f * std::sin(blinkTimer * kBlinkSpeed);
    float alpha = kBlinkAlphaMin + kBlinkAlphaAmplitude * pulse;

    for (int i = 0; i < kMenuCount; ++i) {
        if (i == selectedIndex) {
            menuTexts[i].SetColor({ kSelectedColor.x, kSelectedColor.y, kSelectedColor.z, alpha });
        } else {
            menuTexts[i].SetColor(kUnselectedColor);
        }
    }
    cursorText.SetColor({ kSelectedColor.x, kSelectedColor.y, kSelectedColor.z, alpha });

    // ">" は選択中項目の左に配置（項目ごとに幅が違うため毎フレーム追従）
    const BitmapText& selected = menuTexts[selectedIndex];
    float menuY = kMenuPositionY + kMenuSpacing * static_cast<float>(selectedIndex);
    float textLeft = ((float)WinApp::KclientWidth - selected.GetWidth()) / 2.0f;
    cursorText.SetPosition({ textLeft - kCursorGap - cursorText.GetWidth(), menuY });
}

void TitleScene::UpdateTexts() {
    background->Update();

    titleText.Update();
    for (auto& text : menuTexts) {
        text.Update();
    }
    cursorText.Update();
    guideText.Update();

    settingsHeading.Update();
    for (auto& label : settingsLabels) {
        label.Update();
    }
    settingsHint.Update();
}

void TitleScene::Draw() {
    Framework* framework = Framework::GetInstance();

    // 3D背景 → 暗幕 → テキスト の順（フェードの黒幕はSceneManagerが最後に描く）
    DrawBackground3d();

    // PostProcess::Drawでルートシグネチャが書き換わるため、スプライト描画前にPreDrawし直す
    framework->GetSpriteCommon()->PreDraw();
    background->Draw();

    if (state == State::Settings) {
        DrawSettings();
    } else {
        DrawMain();
    }
}

void TitleScene::DrawShadowPass() {
    // GameSceneと同じ手順。床・壁・ボスを影の落とし手にする
    Object3dCommon* object3dCommon = Framework::GetInstance()->GetObject3dCommon();
    object3dCommon->PreDrawShadow();
    Matrix4x4 lightViewProjection = object3dCommon->GetLightViewProjection();
    floor->DrawShadow(lightViewProjection);
    for (auto& wall : walls) { wall->DrawShadow(lightViewProjection); }
    boss->DrawShadow(lightViewProjection);
    object3dCommon->PostDrawShadow();
}

void TitleScene::DrawBackground3d() {
    Framework* framework = Framework::GetInstance();

    DrawShadowPass();

    postProcess->PreDraw();

    framework->GetSkyboxCommon()->PreDraw();
    skybox->Draw();

    framework->GetObject3dCommon()->PreDraw();
    floor->Draw();
    for (auto& wall : walls) { wall->Draw(); }
    boss->Draw();

    postProcess->PostDraw();
    postProcess->Draw();
}

void TitleScene::DrawMain() {
    titleText.Draw();
    for (auto& text : menuTexts) {
        text.Draw();
    }
    cursorText.Draw();
    guideText.Draw();
}

void TitleScene::DrawSettings() {
    settingsHeading.Draw();
    for (auto& label : settingsLabels) {
        label.Draw();
    }
    settingsMenu.Draw();
    settingsHint.Draw();
}

void TitleScene::Finalize() {
    // タイトル用カメラを残すとGameSceneでもアクティブのままになりMainCameraが使われないため、必ず外す
    CameraManager::GetInstance()->RemoveCamera(kCameraName);
    camera = nullptr;
}
