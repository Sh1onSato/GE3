#include "TitleScene.h"
#include "GameScene.h"
#include "GameSettings.h"
#include "SceneManager.h"
#include "Framework.h"
#include "WinApp.h"
#include "Input.h"
#include <cmath>

void TitleScene::Initialize() {
    Framework* framework = Framework::GetInstance();

    background = std::make_unique<Sprite>();
    background->Initialize(framework->GetSpriteCommon(), "white");
    background->SetPosition({ 0.0f, 0.0f });
    background->SetSize({ (float)WinApp::KclientWidth, (float)WinApp::KclientHeight });
    background->SetColor({ 0.05f, 0.05f, 0.08f, 1.0f });

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

    settingsMenu.Initialize(framework->GetSpriteCommon(), items);

    promptBar = std::make_unique<Sprite>();
    promptBar->Initialize(framework->GetSpriteCommon(), "white");
    promptBar->SetPosition(kPromptBarPosition);
    promptBar->SetSize(kPromptBarSize);
    promptBar->SetColor({ 1.0f, 1.0f, 0.2f, 1.0f });
}

void TitleScene::Update() {
    Framework* framework = Framework::GetInstance();
    Input* input = framework->GetInput();

    // 常時操作可能（GameSceneのようなPキー開閉トグルは無し）
    settingsMenu.Update(input, true);

    // 「準備完了」を示す点滅バー：sin波でアルファを継続的に揺らす
    blinkTimer += 1.0f / 60.0f;
    float alpha = 0.3f + 0.7f * (0.5f + 0.5f * std::sin(blinkTimer * kBlinkSpeed));
    promptBar->SetColor({ 1.0f, 1.0f, 0.2f, alpha });

    background->Update();
    promptBar->Update();

    if (input->TriggerKey(DIK_RETURN)) {
        sceneManager->ChangeScene(new GameScene());
    }
}

void TitleScene::Draw() {
    Framework* framework = Framework::GetInstance();

    framework->GetSpriteCommon()->PreDraw();
    background->Draw();
    settingsMenu.Draw();
    promptBar->Draw();
}

void TitleScene::Finalize() {
}
