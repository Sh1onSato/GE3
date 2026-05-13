#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "Object3d.h"
#include "Model.h"
#include "Camera.h"
#include "Calculation.h"

/// <summary>
/// ゲーム本編シーン
/// </summary>
class GameScene : public BaseScene {
public:
	// 初期化
	void Initialize() override;
	// 終了処理
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;

private:
	// シーン固有のオブジェクト
	Camera* camera = nullptr;
	Model* model = nullptr;
	Object3d* object3d = nullptr;
	Object3d* object3d2 = nullptr;
	Sprite* uvChecker = nullptr;
	Sprite* monsterBall = nullptr;

	class ParticleManager* particleManager = nullptr;
	class ParticleEmitter* particleEmitter = nullptr;
};
