#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "Object3d.h"
#include "Skybox.h"
#include "Model.h"
#include "Camera.h"
#include "Calculation.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include <memory>
#include <vector>

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
	// 当たり判定チェック
	bool CheckCollision(const Vector3& pos, float* outMaxY = nullptr);

	// シーン固有のオブジェクト
	Camera* camera = nullptr; 
	std::unique_ptr<Model> model = nullptr;
	std::unique_ptr<Model> cubeModel = nullptr; // 追加：キューブモデル
	std::unique_ptr<Object3d> object3d = nullptr;
	std::unique_ptr<Object3d> object3d2 = nullptr;
	std::unique_ptr<Object3d> floor = nullptr;    // 追加：床
	std::unique_ptr<Skybox> skybox = nullptr;     // 追加：スカイボックス
	
	std::unique_ptr<Sprite> uvChecker = nullptr;
	std::unique_ptr<Sprite> monsterBall = nullptr;
	std::unique_ptr<Sprite> reticle = nullptr; // 追加：レティクル

	std::unique_ptr<ParticleManager> particleManager = nullptr;
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;

	// FPS操作用
	float mouseSensitivity = 0.002f; // マウス感度
	bool isLookPaused = false;       // 視点移動のポーズフラグ

	// 物理演算用
	Vector3 playerPos = { 0, 0, 0 }; // 足元の位置
	Vector3 velocity = { 0, 0, 0 };  // 現在の速度
	bool isOnGround = false;         // 接地フラグ
	float eyeHeight = 1.5f;          // 目線の高さ
	float collisionRadius = 0.5f;    // 体の横幅（判定用）
};
