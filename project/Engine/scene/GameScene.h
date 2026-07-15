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
#include "PostProcess.h"
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

	// 射撃処理
	void FireShot();
	// 弾痕デカールをリングバッファで生成（最古のものを上書き）。hitAABBは着弾面の端からのはみ出し防止に使う
	void SpawnBulletDecal(const Vector3& position, const Vector3& normal, const AABB& hitAABB);
	// 視点武器の追従・リコイル演出の更新
	void UpdateWeaponViewModel();
	// 視点武器の描画（常に最前面）
	void DrawWeaponViewModel();
	// カメラシェイクを開始する（発砲時は小さく、ヒット時は大きく、のように呼び出し側で強さを使い分ける）
	void TriggerCameraShake(float duration, float strength);
	// オブジェクトからAABBを取得する補助関数
	AABB GetAABB(const Object3d& object);

	// ヒット演出用の構造体
	struct HitFlash {
		Object3d* object;
		float timer;
		Vector4 originalColor;
	};
	std::vector<HitFlash> hitFlashes;

	// シーン固有のオブジェクト
	Camera* camera = nullptr; 
	std::unique_ptr<Model> model = nullptr;
	std::unique_ptr<Model> cubeModel = nullptr; // 追加：キューブモデル
	std::unique_ptr<Object3d> object3d = nullptr;
	std::unique_ptr<Object3d> object3d2 = nullptr;
	std::unique_ptr<Object3d> floor = nullptr;    // 追加：床
	std::unique_ptr<Skybox> skybox = nullptr;     // 追加：スカイボックス
	std::vector<std::unique_ptr<Object3d>> walls; // 追加：複数の壁

	// 弾痕デカール用
	std::unique_ptr<Model> planeModel = nullptr;
	static const int kMaxBulletDecals = 32;
	std::vector<std::unique_ptr<Object3d>> bulletDecals;
	int nextDecalIndex = 0;

	// 視点武器（プレースホルダー：cubeModel流用）
	std::unique_ptr<Object3d> weaponBody = nullptr;
	std::unique_ptr<Object3d> weaponBarrel = nullptr;
	Vector3 weaponBodyLocalOffset = { 0.22f, -0.20f, 0.45f };
	Vector3 weaponBarrelLocalOffset = { 0.22f, -0.18f, 0.62f };
	float weaponRecoilTimer = 0.0f;
	static constexpr float kRecoilDuration = 0.12f;   // リコイル演出の継続時間(秒)
	static constexpr float kRecoilKickAmount = 0.08f; // 発砲直後にカメラ側へ引く量(m)

	// カメラシェイク（発砲・ヒット時に視点をランダムに揺らす演出）
	float cameraShakeTimer = 0.0f;    // 残り時間(秒)。0になると揺れは止まる
	float cameraShakeDuration = 0.0f; // 今回の揺れの総時間(秒)。強さの減衰計算に使う
	float cameraShakeStrength = 0.0f; // 揺れの最大角度(ラジアン)
	static constexpr float kFireShakeDuration = 0.06f; // 発砲時：小さく素早い揺れ
	static constexpr float kFireShakeStrength = 0.006f;
	static constexpr float kHitShakeDuration = 0.08f;  // ヒット時：旧・発砲時と同程度の揺れ
	static constexpr float kHitShakeStrength = 0.01f;

	std::unique_ptr<Sprite> uvChecker = nullptr;
	std::unique_ptr<Sprite> monsterBall = nullptr;
	std::unique_ptr<Sprite> reticle = nullptr; // 追加：レティクル

	std::unique_ptr<PostProcess> postProcess = nullptr;
	// 点・線・面を同時に出せるよう、描画タイプごとにParticleManagerを分けて持つ
	std::unique_ptr<ParticleManager> particleManagerLine = nullptr;
	std::unique_ptr<ParticleManager> particleManagerPoint = nullptr;
	std::unique_ptr<ParticleManager> particleManagerTriangle = nullptr;
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
