#pragma once
#include "BaseScene.h"
#include "Sprite.h"

/// <summary>
/// タイトルシーン
/// </summary>
class TitleScene : public BaseScene {
public:
	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	Sprite* titleSprite = nullptr;
};
