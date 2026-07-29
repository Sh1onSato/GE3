#pragma once
#include "BaseScene.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "SettingsMenu.h"
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
	// 当たり判定チェック（radius省略時はメンバのcollisionRadiusを使う。ボスなど別サイズの判定に使う場合は明示的に渡す）
	// outPushDirを渡すと、衝突した中で最も近い（＝めり込みが深い）障害物について、
	// 「障害物の最近接点→球の中心」への正規化ベクトル（＝壁から押し出す方向・衝突面の法線）を返す
	bool CheckCollision(const Vector3& pos, float* outMaxY = nullptr, float radius = -1.0f, Vector3* outPushDir = nullptr);

	// 武器モード（ライフル／ポータルガン）
	enum class WeaponMode { kRifle, kPortalGun };

	// ワープポータル1つ分の状態
	struct Portal {
		bool isPlaced = false;
		Vector3 position = {};
		Vector3 normal = { 0, 1, 0 };
		Quaternion orientation = {};
		std::unique_ptr<Object3d> visual;
	};

	// 射撃処理
	void FireShot();
	// 弾痕デカールをリングバッファで生成（最古のものを上書き）。hitAABBは着弾面の端からのはみ出し防止に使う
	void SpawnBulletDecal(const Vector3& position, const Vector3& normal, const AABB& hitAABB);
	// targetsの中から最も近いレイヒットを探す（FireShot/PlacePortalで共用）
	bool RaycastClosest(const Ray& ray, const std::vector<Object3d*>& targets, RaycastHit* outHit, AABB* outAABB, Object3d** outObject = nullptr);
	// ポータルを設置する（カメラ正面へレイキャストし、床・壁にヒットした位置に置く）
	void PlacePortal(Portal& portal);
	// ポータル同士のテレポート判定・実行（移動処理より後、playerPos確定後に呼ぶ）。
	// preCollisionVelocityYはY軸の衝突判定でvelocity.yが0にリセットされる前の値を渡すこと
	void UpdatePortalTeleport(float preCollisionVelocityY);
	// ベクトルvを、fromの姿勢からtoの姿勢へ180度反転込みで変換する
	Vector3 TransformThroughPortal(const Portal& from, const Portal& to, const Vector3& v);
	// 視点武器の追従・リコイル演出の更新
	void UpdateWeaponViewModel();
	// 視点武器の描画（常に最前面）
	void DrawWeaponViewModel();
	// カメラシェイクを開始する（発砲時は小さく、ヒット時は大きく、のように呼び出し側で強さを使い分ける）
	void TriggerCameraShake(float duration, float strength);
	// オブジェクトからAABBを取得する補助関数
	AABB GetAABB(const Object3d& object);
	// プレイヤーがダメージを受けた時の処理（HP減算・0未満クランプ・被弾シェイク）
	void TakeDamage(float amount);
	// ボスがダメージを受けた時の処理（HP減算・0未満クランプのみ。シェイクは呼び出し元のFireShotが担当）
	void DamageBoss(float amount);

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

	// 敵キャラクター（第一段階：静的な的）
	struct EnemyTarget {
		std::unique_ptr<Object3d> object;
		bool isActive = true;
		float respawnTimer = 0.0f;
	};
	std::vector<EnemyTarget> enemies;
	static constexpr float kEnemyRespawnDelay = 3.0f; // 撃たれてから復活までの秒数(秒)

	// ボス（1v1ボス戦フェーズ2：HP/UIの土台。移動・攻撃AIは次フェーズ）
	std::unique_ptr<Object3d> boss = nullptr;
	float bossHP = kBossMaxHP;
	static constexpr float kBossMaxHP = 300.0f;        // 仮値（プレイヤーの3倍程度）
	static constexpr float kBossDamagePerShot = 15.0f; // 1発あたりのダメージ（仮値、20発で撃破）

	// ボスの移動AI（常に追いかける。距離とHPで速度を変化させる）
	Vector3 bossPos = { 0.0f, 0.0f, 20.0f }; // 足元基準（playerPosと同じ規約）
	static constexpr float kBossHalfHeight = 2.5f;          // 底面を床(y=0)に合わせるための高さオフセット
	static constexpr float kBossCollisionRadius = 2.0f;     // 横幅（ボスのscale.x/zに合わせる）
	static constexpr float kBossStopDistance = 3.0f;        // これより近づいたら停止（次フェーズの攻撃間合いを想定）
	static constexpr float kBossFarDistance = 15.0f;        // これより遠いと最高速度になる
	static constexpr float kBossSpeedNear = 0.05f;          // 間合いに入った時の速度下限
	static constexpr float kBossSpeedFar = 0.12f;           // 遠い時の速度上限
	static constexpr float kBossEnrageMaxMultiplier = 1.6f; // HP0時、速度に掛かる倍率の上限
	bool bossAvoiding = false;   // 直進が塞がっていて、回避方向を保持中かどうか
	Vector3 bossAvoidDir = {};   // 保持中の回避（滑り）方向
	float bossStuckTimer = 0.0f; // 直進・スライドどちらも失敗して動けていない継続時間
	static constexpr float kBossStuckRetreatDelay = 0.3f; // これ以上動けない状態が続いたら後退を試みる

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
	static constexpr float kPlayerDamageShakeDuration = 0.15f; // 被弾時：与ダメージより大きめに揺らす
	static constexpr float kPlayerDamageShakeStrength = 0.02f;

	// プレイヤーHP（1v1ボス戦に向けた土台。現時点ではボス未実装のためNキーのテストダメージのみが減算源）
	float playerHP = kPlayerMaxHP;
	static constexpr float kPlayerMaxHP = 100.0f;
	static constexpr float kTestDamageAmount = 10.0f; // Nキーで受けるテストダメージ量

	std::unique_ptr<Sprite> uvChecker = nullptr;
	std::unique_ptr<Sprite> monsterBall = nullptr;
	std::unique_ptr<Sprite> reticle = nullptr; // 追加：レティクル
	std::unique_ptr<Sprite> hpBarBackground = nullptr; // HPバーの背景（暗色の枠）
	std::unique_ptr<Sprite> hpBarFill = nullptr;        // HPバーの残量表示（HP比率に応じて幅が縮む）
	static constexpr float kHpBarMaxWidth = 300.0f;
	static constexpr float kHpBarHeight = 24.0f;
	static constexpr Vector2 kHpBarPosition = { 20.0f, 670.0f }; // 画面左下
	BitmapText playerHpText; // HPバーの隣に実際の数値を表示（ImGui非依存のビットマップフォント基盤の動作確認を兼ねる）
	static constexpr Vector2 kHpTextPosition = { kHpBarPosition.x + kHpBarMaxWidth + 12.0f, kHpBarPosition.y };

	// ImGui非依存の設定メニュー（Pキーでのポーズ中のみ操作可能。Releaseビルドでもポストエフェクトを調整できる）
	SettingsMenu settingsMenu;

	std::unique_ptr<Sprite> bossHpBarBackground = nullptr;
	std::unique_ptr<Sprite> bossHpBarFill = nullptr;
	static constexpr float kBossHpBarMaxWidth = 500.0f;
	static constexpr float kBossHpBarHeight = 20.0f;
	static constexpr Vector2 kBossHpBarPosition = { 390.0f, 20.0f }; // 画面上部中央(1280幅基準)

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
	// 今フレームの水平移動方向（WASD入力由来）。UpdatePortalTeleport()から参照するためメンバ化
	Vector3 moveDir = { 0, 0, 0 };

	// ワープポータル
	WeaponMode weaponMode = WeaponMode::kRifle;
	Portal portalA; // 青
	Portal portalB; // オレンジ
	static constexpr float kPortalRadius = 1.0f;
	static constexpr float kPortalTeleportCooldown = 0.3f;
	float portalCooldownTimer = 0.0f;

	// テレポート直後の勢い継承（移動方向に一定時間だけ加算し、徐々に減衰させる）
	Vector3 portalExitVelocity = { 0, 0, 0 };
	float portalExitVelocityTimer = 0.0f;
	static constexpr float kPortalExitVelocityDuration = 0.25f;
};
