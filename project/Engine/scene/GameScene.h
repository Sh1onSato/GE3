#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "Object3d.h"
#include "Model.h"
#include "Camera.h"
#include "Calculation.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include <memory>

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
	Camera* camera = nullptr; // カメラはCameraManagerが所有しているため、参照用として生ポインタで持つ
	std::unique_ptr<Model> model = nullptr;
	std::unique_ptr<Object3d> object3d = nullptr;
	std::unique_ptr<Object3d> object3d2 = nullptr;
	std::unique_ptr<Sprite> uvChecker = nullptr;
	std::unique_ptr<Sprite> monsterBall = nullptr;

	std::unique_ptr<ParticleManager> particleManager = nullptr;
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;
};
