#pragma once
#include "BaseScene.h"
#include "ArenaLayout.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "PauseTuningMenu.h"
#include "Object3d.h"
#include "Skybox.h"
#include "Model.h"
#include "Camera.h"
#include "Calculation.h"
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "ScreenEffectController.h"
#include <memory>
#include <vector>
#include <array>

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
		int platformIndex = -1; // 設置面がデータ足場ならそのインデックス（足場が消えたらポータルも消す）。-1なら床・壁
	};

	// ポータルの組（a↔bがつながる）。1組目=青/オレンジ、2組目(P3のみ)=緑/ピンク
	struct PortalPair {
		Portal a; // 左クリックで置く側
		Portal b; // 右クリックで置く側
	};
	static constexpr int kMaxPortalPairs = 2;

	// 射撃処理
	void FireShot();
	// 弾痕デカールをリングバッファで生成（最古のものを上書き）。hitAABBは着弾面の端からのはみ出し防止に使う
	void SpawnBulletDecal(const Vector3& position, const Vector3& normal, const AABB& hitAABB);
	// targetsの中から最も近いレイヒットを探す（FireShot/PlacePortalで共用）
	bool RaycastClosest(const Ray& ray, const std::vector<Object3d*>& targets, RaycastHit* outHit, AABB* outAABB, Object3d** outObject = nullptr);
	// ポータルを設置する（カメラ正面へレイキャストし、床・壁にヒットした位置に置く）
	// modelは置くポータルに使うモデル（組・a/bごとに違う）
	void PlacePortal(Portal& portal, const std::shared_ptr<Model>& model);
	// いま置ける組の数（P3のみ2組、それ以外は1組）
	int ActivePortalPairCount() const { return bossPhase == BossPhase::kP3 ? 2 : 1; }
	// 同じ組の相方を返す（portalがどの組にも属さなければnullptr）
	Portal* FindPartner(const Portal& portal);
	// 全ポータル（kMaxPortalPairs*2個）を列挙する
	std::array<Portal*, kMaxPortalPairs * 2> AllPortals();
	// ポータルを置いていない状態に戻す（見た目は縮小して隠す）
	static void ClearPortal(Portal& portal);
	// ポータル同士のテレポート判定・実行（移動処理より後、playerPos確定後に呼ぶ）。
	// preCollisionVelocityYはY軸の衝突判定でvelocity.yが0にリセットされる前の値を渡すこと
	void UpdatePortalTeleport(float preCollisionVelocityY);
	// ベクトルvを、fromの姿勢からtoの姿勢へ180度反転込みで変換する
	Vector3 TransformThroughPortal(const Portal& from, const Portal& to, const Vector3& v);
	// 描画用の行列(WVP)・HPバー等UIの更新（ポーズ中もゲーム進行と無関係に毎フレーム必要な処理）
	void UpdateRenderObjects();
	// Debugビルド専用のImGui調整パネル（ポーズ中のみ表示）
	void UpdateDebugGui();
	// 視点武器の追従・リコイル演出の更新
	void UpdateWeaponViewModel();
	// ボス描画用5パーツ（くさび4＋コア）をbossの芯transformに合わせ、開閉・発光を反映してUpdateする
	void UpdateBossParts();
	// 5パーツ全部に同じ実体化ディゾルブをかける（bossの芯にもかける）
	void SetBossDissolve(float height, float edgeWidth, float time, float blockStrength = 1.0f);
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
	// 勝敗演出（gameOver）中にEnterキーで呼ばれる。プレイヤー/ボス/ポータル等をスポーン直後の状態へ戻す
	void RestartGame();
	// 開始演出（READY→FIGHT）を最初から始める。fullVersion=trueで黒帯・ボスHPチャージ・視点ロック付き（初回開始用）
	void StartIntro(bool fullVersion);
	// 開始演出の見た目（文字の透明度・黒帯・ボスHPバーのチャージ）を経過時間に合わせて更新する
	void UpdateIntroVisual();
	// フル版の開始演出：READY中はボスを見上げる演出用カメラにし、黒帯が引く間にプレイヤー視点へ戻す
	void UpdateIntroCamera();
	// 演出用カメラ中かどうか（銃・レティクルを隠す判定に使う）
	bool IsIntroCameraActive() const { return introPhase == IntroPhase::kReady && introIsFull; }
	// ボスが実体化演出の途中か（影パスを止める判定に使う）
	// ボスが無敵か。止まって溜めている「特別な行動」の間は、撃っても効かない（撃つと弾かれた表示になる）。
	// アリーナ再構築の溜めは、フェーズ移行の直後に最優先で来るため、移行演出から続けて無敵にしている
	// （以前は再構築の溜めだけ無敵でなく、待っているのにゲージが減って見えた。2026/11/11）
	bool IsBossInvulnerable() const {
		return bossAttackState == BossAttackState::kPhaseShift
			|| bossAttackState == BossAttackState::kTeleportOut
			|| bossAttackState == BossAttackState::kTeleportIn
			|| bossAttackState == BossAttackState::kRebuildWindup;
	}
	bool IsBossMaterializing() const {
		return (introPhase == IntroPhase::kReady && introElapsed < kIntroMaterializeStart + kIntroMaterializeDuration)
			|| bossAttackState == BossAttackState::kTeleportOut
			|| bossAttackState == BossAttackState::kTeleportIn;
	}
	// 開始演出の文字を差し替えて画面中央に配置する
	void SetIntroText(const std::string& text, const Vector4& color);

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
	// ボス用モデル（仮）：実体化演出の横ズレ（頂点シェーダー）が帯状に効くよう、高さ方向に分割したcube。本番モデル追加時に差し替える
	std::unique_ptr<Model> bossModel = nullptr;
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
	// bossは当たり判定・位置・回転・実体化状態の「芯」として残し、自身は描画しない。
	// 見た目はbossPartsの5パーツ（AI/AABB/レイキャスト側を一切変えずにモデルだけ差し替えるため）
	std::unique_ptr<Object3d> boss = nullptr;
	enum BossPart { kBossWedgeTop, kBossWedgeBottom, kBossWedgeLeft, kBossWedgeRight, kBossCore, kBossPartCount };
	std::array<std::unique_ptr<Object3d>, kBossPartCount> bossParts;
	float bossOpenAmount = 0.0f; // 0=閉、1=開（予備動作〜攻撃中に開く）
	static constexpr float kBossOpenSpeed = 4.0f;                 // 1秒あたりのbossOpenAmount変化量
	static constexpr float kBossWedgeOpenDistance = 1.3f;         // くさびを開く距離（上・左・右）
	static constexpr float kBossWedgeOpenDistanceBottom = 0.8f;   // 下だけ床にめり込まないよう短くする
	Vector3 bossWarningEmissive = { 0.0f, 0.0f, 0.0f };           // 予備動作の警告色（旧boss->SetColorの代わりにくさびの発光で表現）
	static constexpr float kBossWarningEmissiveIntensity = 0.6f;
	float bossHitFlashTimer = 0.0f;                               // 被弾フラッシュの残り時間（くさびを赤く発光）
	float bossHealFlashTimer = 0.0f;                              // 回復の光の残り時間（くさびをデータの色に発光）
	static constexpr float kBossHealFlashDuration = 0.3f;
	static constexpr float kBossHealFlashIntensity = 0.9f;
	static constexpr float kBossHitFlashDuration = 0.2f;
	static constexpr float kBossHitFlashIntensity = 0.8f;
	float bossHP = kBossMaxHP;
	static constexpr float kBossMaxHP = 300.0f; // 仮値（プレイヤーの3倍程度）

	// プレイヤーの射撃ダメージ（距離で線形減衰。近距離ほど高ダメージ＝ショットガン的な仕様）
	// 1発が大きすぎて簡単だったため下げている（2026/11/11、25/8 → 12/4 → 「削りが速すぎる」で 9/3）。
	// コア命中はこの kBossCoreDamageMultiplier 倍
	static constexpr float kBossDamageMax = 9.0f;           // 至近距離での最大ダメージ
	static constexpr float kBossDamageMin = 3.0f;           // 遠距離での最小ダメージ
	static constexpr float kBossDamageFalloffStart = 5.0f;  // これ以下の距離は常に最大ダメージ
	static constexpr float kBossDamageFalloffEnd = 20.0f;   // これ以上の距離は常に最小ダメージ

	// ボスの移動AI（常に追いかける。距離とHPで速度を変化させる）
	Vector3 bossPos = ArenaLayout::kBossInitialPos; // 足元基準（playerPosと同じ規約）
	static constexpr float kBossHalfHeight = ArenaLayout::kBossHalfHeight; // 底面を床(y=0)に合わせるための高さオフセット
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

	// ボスの攻撃AI（距離しきい値で近接突進／遠距離グレネードを自動切替）
	// kChasing中は既存の追跡・障害物回避ロジックで移動を続けながらbossAttackTimerを減算し、
	// 0以下になったタイミングでその時の距離に応じてkMeleeWindup/kRangedWindupへ遷移する
	// （＝「クールダウン中は棒立ち」ではなく、既存の追跡AIをそのまま活かして常に動き続ける設計）
	enum class BossAttackState { kChasing, kMeleeWindup, kMeleeDash, kRangedWindup, kRangedBurst, kPhaseShift, kLaserWindup, kLaserSweep, kTeleportOut, kTeleportIn, kRebuildWindup, kShield, kFloorWipeWindup };
	BossAttackState bossAttackState = BossAttackState::kChasing;

	// ボスのフェーズ（HP比率で段階的に移行する。P3固有の挙動は段階2で追加予定）
	enum class BossPhase { kP1, kP2, kP3 };
	BossPhase bossPhase = BossPhase::kP1;
	// 「フェーズ1がぱっとしない」とのことで早めに変化させる（2026/11/11、旧 0.6 / 0.25）
	static constexpr float kBossPhase2HpRatio = 0.7f;   // HP比率がこれを下回ったらP2へ
	static constexpr float kBossPhase3HpRatio = 0.45f;  // HP比率がこれを下回ったらP3へ
	static constexpr float kBossPhaseShiftDuration = 1.2f;       // 移行演出（無敵）の長さ
	static constexpr float kBossPhaseShiftShakeDuration = 0.35f;
	static constexpr float kBossPhaseShiftShakeStrength = 0.03f;
	static constexpr float kBossPhaseShiftGlitch = 1.0f;         // 移行開始時のグリッチ強度
	static constexpr float kBossPhaseShiftGlitchDuration = 0.5f; // グリッチが消えるまでの時間
	static constexpr float kBossPhase2IdleOpen = 0.5f;           // P2以降、追跡中のくさびの開き具合
	void StartBossPhaseShift(BossPhase next);
	void UpdatePhaseText();

	// "PHASE 2"/"PHASE 3"の文字
	BitmapText phaseText;
	float phaseTextTimer = 0.0f;
	static constexpr int kPhaseTextMaxChars = 7;
	static constexpr float kPhaseTextScale = 5.0f;
	static constexpr float kPhaseTextDuration = 1.5f;
	static constexpr float kPhaseTextFadeDuration = 0.5f; // 最後のこの時間でアルファを下げて消す
	static constexpr float kPhaseTextPositionY = 320.0f;
	static constexpr Vector4 kPhaseTextColor = { 0.85f, 0.3f, 1.0f, 1.0f };

	// ===== アリーナ再構築（P2以降。床の一部を危険地帯にし、空中にデータ足場を実体化させる） =====
	static constexpr float kArenaRebuildInterval = 20.0f;       // 再構築の間隔[秒]
	static constexpr float kArenaRebuildWindupDuration = 1.0f;  // 予備動作の長さ
	static constexpr Vector3 kArenaRebuildWarningEmissive = { 0.3f, 1.0f, 0.8f }; // 予兆の色（データの色）
	static constexpr float kArenaRebuildGlitch = 0.35f;         // 予備動作の終わりに向けて強めるグリッチの最大強度
	float arenaRebuildTimer = 0.0f;                             // 0以下で次の攻撃選択時に再構築へ入る
	void StartArenaRebuild();   // 床の破損とデータ足場の出現を同時に始める
	void UpdateArenaRebuild();  // タイマー・破損マス・足場・床ダメージの更新（ボスAIが動いている間だけ呼ぶ）
	void StopArenaRebuild();    // 破損マスと足場を全て非表示・判定なしに戻す

	// ===== 床全体の消去（P2以降の大技。データ足場の上にいないと大ダメージ＝ポータルでの退避が必須になる） =====
	static constexpr float kFloorWipeInterval = 35.0f;          // 次に使えるようになるまでの間隔[秒]
	static constexpr float kFloorWipeFirstDelay = 15.0f;        // P2に入ってから1回目までの待ち[秒]
	static constexpr float kFloorWipeWindupDuration = 3.5f;     // 溜め（予兆）の長さ
	static constexpr float kFloorWipeDamage = 40.0f;
	static constexpr float kFloorWipeFlashDuration = 0.25f;     // 発動時に床が白く光る時間
	static constexpr float kFloorWipeFlashIntensity = 3.0f;
	static constexpr float kFloorWipeBlinkPeriodStart = 0.4f;   // 溜めの点滅周期：最初
	static constexpr float kFloorWipeBlinkPeriodEnd = 0.08f;    // 溜めの点滅周期：最後
	static constexpr float kFloorWipeBlinkIntensityStart = 0.3f;
	static constexpr float kFloorWipeBlinkIntensityEnd = 1.5f;
	static constexpr float kFloorWipeWindupGlitchMax = 0.3f;    // 溜めの終わりに向けて強めるグリッチの最大強度
	static constexpr float kFloorWipeGlitchOnFire = 1.0f;       // 発動時のグリッチ強度
	static constexpr float kFloorWipeGlitchOnFireDuration = 0.4f;
	static constexpr float kFloorWipeShakeDuration = 0.4f;
	static constexpr float kFloorWipeShakeStrength = 0.03f;
	static constexpr Vector3 kFloorWipeWarningColor = { 1.0f, 0.1f, 0.1f };
	static constexpr Vector3 kFloorWipeFlashColor = { 1.0f, 1.0f, 1.0f };
	float floorWipeTimer = 0.0f;           // 0以下かつ足場があれば次の攻撃選択で使う（P2以降のボスAIが動いている間だけ減る）
	float floorWipeFlashTimer = 0.0f;      // 発動後の白い閃光の残り時間
	float floorWipeGlitchTimer = 0.0f;     // 発動後のグリッチの残り時間
	bool IsAnyDataPlatformActive() const;  // 乗れる状態のデータ足場が1つでもあるか
	void FireFloorWipe();                  // 溜め終了：ダメージ判定と閃光・グリッチ・シェイク
	void UpdateFloorWipeEffect(float elapsed);   // 溜め中の床点滅・WARNING文字・グリッチ
	void UpdateFloorWipeFlash();           // 発動後の白い閃光とグリッチの減衰
	void StopFloorWipe();                  // 溜め・閃光を打ち切り、床の発光・文字・グリッチを戻す

	// "WARNING"の文字（床全体の消去の溜め中だけ点滅表示）
	BitmapText warningText;
	bool warningTextVisible = false;
	static constexpr int kWarningTextMaxChars = 7;
	static constexpr float kWarningTextScale = 5.0f;
	static constexpr float kWarningTextPositionY = 200.0f;
	static constexpr float kWarningTextBlinkInterval = 0.12f;
	static constexpr Vector4 kWarningTextColor = { 1.0f, 0.1f, 0.1f, 1.0f };

	// ===== ワープ直後の強化（ポータルで移動した直後だけ、与ダメージ増加＋短い無敵） =====
	float warpBoostTimer = 0.0f;
	float warpInvulnTimer = 0.0f;
	static constexpr float kWarpBoostDuration = 3.0f;
	static constexpr float kWarpInvulnDuration = 0.5f;
	static constexpr float kWarpBoostDamageMultiplier = 1.5f;
	static constexpr Vector4 kReticleWarpBoostColor = { 0.2f, 0.9f, 1.0f, 1.0f };

	// --- 床の破損（見た目だけのマス。当たり判定の床は1枚のまま） ---
	static constexpr float kFloorTileSize = 4.0f;               // 1マスの一辺[m]
	static constexpr float kFloorTileHalfSize = kFloorTileSize * 0.5f;
	static constexpr int kFloorTileCountPerSide = 15;           // 15x15マス
	static constexpr float kFloorTileOriginCenter = -28.0f;     // 最初のマスの中心座標（x,z共通。-28,-24,...,28）
	// 1回の再構築で破損させるマス数（ユーザー「もうちょい増やしていい」で 6→10、近く 3→4。2026/11/11）
	static constexpr int kCorruptTileCount = 10;                // 1回の再構築で破損させるマス数（225マス中）
	static constexpr int kCorruptTilesNearPlayer = 4;           // うちプレイヤー付近から選ぶ数
	static constexpr float kCorruptNearRadius = 8.0f;           // 「付近」とみなすXZ距離
	static constexpr float kCorruptTileLift = 0.02f;            // 床の上面より少しだけ浮かせる量（重ね表示）
	static constexpr float kCorruptTileCenterY = -1.0f;         // floor_tile.glb(厚さ2m)の中心Y。上面がy=0になる
	static constexpr Vector3 kCorruptTileScale = { 2.0f, 1.0f, 2.0f };
	static constexpr float kCorruptWarningDuration = 1.5f;      // 点滅（ダメージなし）
	static constexpr float kCorruptActiveDuration = 8.0f;       // 危険地帯として有効な時間
	static constexpr float kCorruptBlinkInterval = 0.1f;        // 警告点滅の切り替え間隔
	static constexpr Vector3 kCorruptGlitchEmissive = { 1.0f, 0.1f, 0.1f };
	static constexpr float kCorruptGlitchIntensityBase = 1.2f;  // 赤いちらつきの基準強度
	static constexpr float kCorruptGlitchIntensityAmp = 0.8f;   // ちらつきの振れ幅
	static constexpr float kCorruptGlitchFlickerSpeed = 40.0f;  // ちらつきの速さ
	static constexpr float kCorruptGlitchPhaseStep = 1.7f;      // マスごとに位相をずらす量
	static constexpr float kCorruptDamageMaxHeight = 0.5f;      // 足元がこれ未満（＝地上）の時だけダメージ
	static constexpr float kCorruptDamageInterval = 0.5f;
	static constexpr float kCorruptDamagePerTick = 5.0f;
	// P3では、プレイヤーが崩壊床でダメージを受けるたびにボスがこれだけ回復する（0.5秒ごと＝毎秒6）
	static constexpr float kCorruptBossHealPerTick = 3.0f;
	enum class CorruptState { kHidden, kWarning, kActive };
	struct CorruptTile {
		std::unique_ptr<Object3d> object;
		CorruptState state = CorruptState::kHidden;
		float timer = 0.0f;      // 状態に入ってからの経過時間
		Vector3 center = {};     // マス中心のXZ（yは使わない）
		bool visible = false;    // 点滅用：今フレーム描画するか
	};
	std::array<CorruptTile, kCorruptTileCount> corruptTiles;
	float corruptDamageTimer = 0.0f; // 床ダメージのティック用（1つを共用）

	// --- データ足場（空中。ポータルでしか上がれず、グレネードの爆発で壊れる） ---
	static constexpr int kDataPlatformCount = 3;
	static constexpr int kDataPlatformCandidateCount = 5;
	// 候補位置（板の中心）
	static constexpr std::array<Vector3, kDataPlatformCandidateCount> kDataPlatformPositions = { {
		{ -8.0f, 6.5f, 4.0f }, { 8.0f, 6.5f, 4.0f }, { 0.0f, 7.0f, -4.0f }, { -6.0f, 6.0f, -12.0f }, { 6.0f, 6.0f, -12.0f }
	} };
	// 5m四方・厚さ0.4m（そのまま当たり判定のhalf-extent）。角のブロックが崩れて上の重要度が上がったため、4mから広げた
	// （モデル data_platform.glb もこの scale 用に作ってある。変えるときは Tools/blender/arena.py も合わせる）
	static constexpr Vector3 kDataPlatformScale = { 2.5f, 0.2f, 2.5f };
	static constexpr float kDataPlatformMaterializeDuration = 0.8f;
	static constexpr float kDataPlatformBreakDuration = 0.8f;
	static constexpr float kDataPlatformDissolveEdgeWidth = 0.15f;       // 実体化の境界の発光帯の幅[m]
	static constexpr float kDataPlatformBreakBlinkInterval = 0.06f;      // 崩壊中の点滅間隔
	static constexpr Vector3 kDataPlatformBreakEmissive = { 1.0f, 0.1f, 0.1f };
	static constexpr float kDataPlatformBreakEmissiveIntensity = 1.5f;
	static constexpr const char* kCorruptTileModelFile = "floor_tile.glb";
	static constexpr const char* kDataPlatformModelFile = "data_platform.glb";
	// 足場の中央に立てる柱。地上から足場の上面は見えないため、この柱の側面にポータルを貼って上がる
	// プレイヤーが高い場所（データ足場の上）にいるときのボスの動き。
	// 真下に寄って届かない突進・衝撃波ばかり選んでしまうため、距離を取ってグレネード（足場を壊す）中心にする
	static constexpr float kPlayerHighGroundHeight = 5.5f;          // 足元がこれより上なら「高い場所」（角の上段の上面5.0より上）
	static constexpr float kBossHighGroundKeepDistance = 10.0f;     // 高い場所のプレイヤーとは、水平にこの距離まで近づいたら止まる
	static constexpr float kBossHighGroundRetreatDistance = 8.0f;   // これより近ければ下がる
	static constexpr float kBossHighGroundRetreatSpeed = 0.08f;
	bool IsPlayerOnHighGround() const { return playerPos.y > kPlayerHighGroundHeight; }
	static constexpr Vector3 kDataPylonScale = { 0.5f, 1.25f, 0.5f };  // 1m角・高さ2.5m（そのまま当たり判定のhalf-extent）
	static constexpr const char* kDataPylonModelFile = "data_pylon.glb";
	// 柱の中心＝足場の中心から（足場の厚みの半分＋柱の高さの半分）だけ上
	static constexpr float kDataPylonOffsetY = kDataPlatformScale.y + kDataPylonScale.y;
	struct DataPlatform {
		enum class State { kHidden, kMaterializing, kActive, kBreaking };
		std::unique_ptr<Object3d> object;
		std::unique_ptr<Object3d> pylon; // 足場と同じ状態で出たり消えたりする
		State state = State::kHidden;
		float timer = 0.0f;
		bool visible = false;    // 点滅用：今フレーム描画するか
	};
	std::array<DataPlatform, kDataPlatformCount> dataPlatforms;
	// 当たり判定に入る足場（kActive・kBreaking）をoutへ加える（床・壁と同じ扱いにするため）
	void AppendDataPlatformColliders(std::vector<Object3d*>& out) const;
	// objectが足場ならそのインデックス、そうでなければ-1
	int FindDataPlatformIndex(const Object3d* object) const;
	// 足場を非表示・判定なしに戻し、付いているポータルも消す
	void HideDataPlatform(int index);
	// 爆発の中心から足場のAABBへの最短距離が半径以内のkActive足場をkBreakingにする
	void BreakDataPlatformsByExplosion(const Vector3& center, float radius);

	// コア命中ボーナス（くさびが開いている間だけコアが狙える）
	static constexpr float kBossCoreCenterOffsetY = 0.5f;       // bossPos.y + kBossHalfHeight からのコア中心オフセット
	static constexpr float kBossCoreHalfSize = 0.6f;
	static constexpr float kBossCoreExposedOpenAmount = 0.3f;   // P1でコアが露出したとみなす開き具合
	static constexpr float kBossCoreDamageMultiplier = 1.5f; // ユーザー指定で 2.0→1.3→1.5（2026/11/11）
	static constexpr float kBossCoreHitShakeStrength = 0.015f;

	// レーザー薙ぎ払い（P2以降）
	std::unique_ptr<Object3d> bossLaser;
	bool bossLaserVisible = false;   // trueの間だけ描画（予兆＋薙ぎ）
	bool bossLaserHasHit = false;    // 1回の薙ぎで1回だけダメージを与えるためのフラグ
	float bossLaserCenterYaw = 0.0f; // 薙ぎの中心の向き（予備動作開始時に固定）
	static constexpr float kBossLaserWindupDuration = 0.8f;
	static constexpr float kBossLaserSweepDuration = 1.4f;
	static constexpr float kBossLaserSweepHalfAngle = 1.05f;  // 中心から片側へ振る角度（約60度）
	static constexpr float kBossLaserTelegraphRadius = 0.03f;
	static constexpr float kBossLaserRadius = 0.25f;
	static constexpr float kBossLaserLength = 45.0f;
	static constexpr float kBossLaserHeight = 0.6f;           // 床すれすれ（ジャンプで越えられる）
	static constexpr float kBossLaserDamage = 20.0f;
	static constexpr Vector3 kBossLaserWarningEmissive = { 0.85f, 0.2f, 1.0f };
	// P3は、床すれすれの1本目のすぐ後に、ジャンプした高さを払う2本目（立っていれば当たらない）を逆向きに払う
	float bossLaserHeight = kBossLaserHeight;   // 今のレーザーの高さ（中心）
	bool bossLaserSecondPass = false;           // 2本目（高い方）か
	static constexpr float kBossLaserHighHeight = 2.4f;               // 2本目の高さ（中心）。立っている頭（約1.7m）より上、ジャンプすると当たる
	static constexpr float kBossLaserSecondWindupDuration = 0.5f;     // 2本目の予兆（0.5秒以上の予兆ルール）
	static constexpr Vector3 kBossLaserHighWarningEmissive = { 0.3f, 0.6f, 1.0f };  // 2本目の予兆は青（1本目の紫と見分ける）
	static constexpr float kPlayerHeadMargin = 0.2f;                  // 目線より上の頭の分（レーザーの高さの判定に使う）
	void UpdateBossLaserTransform(float yaw, float radius); // bossLaserの位置・向き・太さを設定する
	void StopBossLaser(); // レーザーを非表示・判定なしに戻す
	float bossAttackTimer = 0.0f;

	// 正面シールド（P1から）：正面から撃ち続けられると張り、正面からの弾のダメージを無効にする
	std::unique_ptr<Object3d> bossShield;
	int bossShieldHitCount = 0;               // 直近の正面命中回数
	float bossShieldHitWindowTimer = 0.0f;    // 最初の正面命中からの残り時間（0以下で回数を数え直す）
	float bossShieldCooldownTimer = 0.0f;     // 張り終わってから次に張れるまでの残り時間
	// 「もうちょい発動条件を緩めていい」で 3秒に5発→3発、クールダウン 8→6秒（2026/11/11）
	static constexpr int kBossShieldTriggerHits = 3;
	static constexpr float kBossShieldTriggerWindow = 3.0f;
	static constexpr float kBossShieldCooldown = 6.0f;
	static constexpr float kBossShieldDuration = 3.5f;
	static constexpr float kBossShieldTurnSpeed = 0.698f;      // 向き直る速さ[rad/秒]（約40度）
	static constexpr float kBossShieldHalfAngle = 1.05f;       // 正面とみなす半角（約60度）
	static constexpr float kBossShieldForwardOffset = 2.4f;
	static constexpr float kBossShieldOffsetY = 0.5f;
	static constexpr float kBossShieldHalfSize = 2.6f;
	static constexpr float kBossShieldAppearDuration = 0.15f;  // 張った瞬間に広がる演出の長さ
	static constexpr float kBossShieldBlockShakeStrength = 0.004f;
	static constexpr Vector3 kBossShieldSparkColor = { 0.4f, 0.8f, 1.0f };  // 弾かれた火花の色（青）
	bool IsFromBossFront(const Vector3& worldPos) const; // ボスの正面から見て worldPos が正面の範囲内か
	bool CanBossStartShield() const;                     // 割り込んでシールドを張ってよい状態か
	void StartBossShield();                              // シールドを張る（予備動作は打ち切る）
	void ResetBossShield();                              // 命中記録・クールダウンを初期化（表示は状態kShieldで決まる）
	void UpdateBossShieldTransform();                    // bossShieldの位置・向き・スケールを設定する

	// --- 組み合わせ攻撃（P2以降、低確率でメインの行動にグレネードを重ねる） ---
	static constexpr float kBossComboChanceP2 = 0.2f;                 // P2で組み合わせ版になる確率
	static constexpr float kBossComboChanceP3 = 0.4f;                 // P3で組み合わせ版になる確率
	static constexpr float kBossComboLaserGrenadeInterval = 0.35f;    // レーザー薙ぎ中のグレネード間隔
	static constexpr int kBossComboLaserGrenadeCount = 4;             // レーザー薙ぎ中のグレネード最大数
	static constexpr float kBossComboDashDropInterval = 0.15f;        // 突進中にグレネードを落とす間隔
	static constexpr float kBossComboDashDropBackOffset = 2.5f;       // 落とす位置（ボスの後ろ＝進行方向の逆）までの距離
	static constexpr int kBossComboDashDropCount = 4;                 // 突進中に落とすグレネードの最大数
	static constexpr float kBossComboShieldGrenadeInterval = 0.8f;    // シールド中のグレネード間隔
	static constexpr float kBossComboBlinkInterval = 0.1f;            // 組み合わせ版の予兆の点滅切り替え間隔
	static constexpr float kBossComboBlinkStrongScale = 1.0f;         // 点滅の強
	static constexpr float kBossComboBlinkWeakScale = 0.25f;          // 点滅の弱
	bool bossComboActive = false;          // 今の行動が組み合わせ版か
	float bossComboTimer = 0.0f;           // 組み合わせのグレネードを撃つまでの残り時間
	int bossComboShotsRemaining = 0;       // 組み合わせで撃てる残り発数（シールドは無制限なので使わない）
	float bossComboBlinkTimer = 0.0f;      // 予兆の点滅用の経過時間
	void RollBossCombo();                  // 行動を始めるとき、確率で組み合わせ版にするか決める（P1は常にfalse）
	void ResetBossCombo();                 // 組み合わせの状態・カウンタを初期化する

	Vector3 bossDashDir = {}; // kMeleeDash中に固定して使い続ける突進方向
	float bossDashCurrentSpeed = 0.0f; // kMeleeDash中、加速しながら変化していく現在の突進速度
	static constexpr float kBossMeleeRange = 10.0f;           // これ以下なら近接突進、これより遠いならグレネード
	static constexpr float kBossMeleeWindupDuration = 0.4f;   // 突進前の予備動作（停止して溜める）時間
	// 突進の数値の強化（長さ0.75・最高速度0.7・判定+0.5）は「強すぎた」ため元に戻した（2026/11/11）。
	// 強化は連続突進と、止まった所の衝撃波だけ残している
	static constexpr float kBossMeleeDashDuration = 0.6f;     // 突進の最大継続時間（不発ならここでタイムアウト）
	static constexpr float kBossDashStartSpeed = 0.15f;       // 突進開始時の速度
	static constexpr float kBossDashMaxSpeed = 0.55f;         // 突進が加速しきった後の最高速度
	static constexpr float kBossDashHitExtraRadius = 0.0f;    // 突進の命中判定に足す半径（強化を戻して0）
	static constexpr int kBossDashCountP1 = 1;                // 1回の突進攻撃で連続して走る回数（P1）
	static constexpr int kBossDashCountP2 = 2;                // 同（P2）
	static constexpr int kBossDashCountP3 = 3;                // 同（P3）
	static constexpr float kBossDashRewindupDuration = 0.5f;  // 2回目以降の突進の予備動作時間（0.5秒以上の予兆ルール）
	static constexpr float kDashShockwaveMaxRadius = 7.0f;    // 突進が止まった所から出る小さな衝撃波が消える半径
	int bossDashRemaining = 0;                                // 今の突進攻撃で、これから走る残り回数
	void EndBossDash();                                       // 1回の突進の終了処理（衝撃波＋次の突進か追跡かの分岐）
	static constexpr float kBossDashAcceleration = 0.025f;    // 突進中、1フレームごとに速度へ加算する加速度
	static constexpr float kBossMeleeDamage = 20.0f;
	static constexpr float kBossRangedWindupDuration = 0.5f;  // グレネード投擲前の予備動作時間
	static constexpr float kBossAttackCooldown = 1.5f;        // 攻撃後、次の攻撃判定が始まるまでの追跡専念時間
	float BossAttackCooldown() const; // フェーズに応じた攻撃後クールダウン（P3は短縮・焦れ中はさらに短縮）

	// --- プレイヤーが壁・カバーの裏に隠れたときのボスの動き ---
	static constexpr float kBossEyeHeight = 3.0f;                  // ボスの胸の高さ（足元からの視線の始点）
	static constexpr float kPlayerChestEyeRatio = 0.7f;            // プレイヤーの胸の高さ（eyeHeightに対する割合）
	static constexpr float kLineOfSightEpsilon = 0.1f;             // 視線判定で、終点手前この距離以内の衝突は無視する
	static constexpr float kBossFlankRepickInterval = 0.5f;        // 回り込み先を選び直す間隔[秒]
	static constexpr float kBossFlankRadius = 9.0f;                // 回り込み先を探す円の半径（プレイヤー中心）
	static constexpr int kBossFlankSampleCount = 16;               // 円周上に取る候補点の数
	static constexpr float kBossFlankArriveDistance = 1.5f;        // 回り込み先にこの距離まで近づいたら止まる
	static constexpr float kBossHiddenTeleportDelay = 2.0f;        // P2以降、隠れ続けてこの秒数でテレポート回り込みを優先する
	static constexpr float kBossHiddenLaserChance = 0.3f;          // 見えないときP2以降でレーザーを選ぶ確率（残りはグレネード）
	static constexpr float kBossFrustrationDelay = 4.0f;           // 隠れ続けてこの秒数で「焦れ」状態になる
	static constexpr float kBossFrustrationCooldownScale = 0.5f;   // 焦れ中の攻撃後クールダウン倍率
	static constexpr int kBossFrustrationGrenadeBurstCount = 5;    // 焦れ中のグレネード連射数
	static constexpr Vector3 kBossFrustrationEmissive = { 1.0f, 0.55f, 0.1f }; // 焦れ中のくさびの発光色（橙）
	static constexpr float kBossFrustrationEmissiveBase = 0.2f;
	static constexpr float kBossFrustrationEmissivePulse = 0.1f;
	static constexpr float kBossFrustrationPulseSpeed = 8.0f;
	float bossHiddenTimer = 0.0f;       // プレイヤーが見えない継続時間[秒]（見えたら0）
	float bossFlankRepickTimer = 0.0f;  // 回り込み先の再選択までの残り時間
	bool bossHasFlankTarget = false;    // bossFlankTargetが有効か
	Vector3 bossFlankTarget = { 0.0f, 0.0f, 0.0f };
	Vector3 PlayerChestPos() const;                                 // 視線判定に使うプレイヤーの胸の位置
	bool HasLineOfSight(const Vector3& from, const Vector3& to);    // 壁・データ足場だけを遮蔽物として2点間に視線が通るか
	bool CanBossSeePlayer();                                        // ボスの胸からプレイヤーの胸が見えるか（高所は常に見える扱い）
	bool TryPickBossFlankTarget();                                  // プレイヤーが見える回り込み先をbossFlankTargetに保存。無ければfalse
	bool IsBossFrustrated() const { return bossHiddenTimer >= kBossFrustrationDelay; }
	void ResetBossHiddenState();                                    // 隠れ関連の状態を初期化（リトライ・勝敗確定用）

	// --- P3固有の挙動 ---
	static constexpr Vector3 kBossPhase3Emissive = { 1.0f, 0.08f, 0.08f }; // P3のくさびの基本発光色（赤）
	static constexpr float kBossPhase3EmissiveBase = 0.25f;
	static constexpr float kBossPhase3EmissivePulse = 0.15f;
	static constexpr float kBossPhase3PulseSpeed = 6.0f;
	static constexpr float kBossPhase3SpeedMultiplier = 1.3f;
	static constexpr float kBossPhase3CooldownScale = 0.6f;
	float bossPhase3PulseTimer = 0.0f; // P3の脈打ち用の累積時間[秒]

	// 短距離テレポート
	static constexpr float kBossFarLaserChance = 0.35f;     // P2以降の遠距離でレーザーを選ぶ確率
	static constexpr float kBossFarTeleportChance = 0.30f;  // P2以降の遠距離でテレポートを選ぶ確率（残りはグレネード）
	static constexpr float kBossTeleportMinDistance = 8.0f;
	static constexpr float kBossTeleportMaxDistance = 12.0f;
	static constexpr float kBossTeleportArenaLimit = 26.0f;
	static constexpr int kBossTeleportMaxTries = 8;
	static constexpr float kBossTeleportOutDuration = 0.6f;
	static constexpr float kBossTeleportInDuration = 0.5f;
	static constexpr float kBossTeleportGlitch = 0.4f;
	static constexpr float kBossTeleportMarkerRadius = 2.2f;
	static constexpr float kBossTeleportMarkerHeight = 0.15f;
	Vector3 bossTeleportDest = { 0.0f, 0.0f, 0.0f };
	std::unique_ptr<Object3d> bossTeleportMarker;
	bool bossTeleportMarkerVisible = false;
	bool TryPickBossTeleportDest(bool requireLineOfSight = false); // 行き先を探してbossTeleportDestに保存する。見つからなければfalse（trueならプレイヤーから見える場所のみ）
	void StopBossTeleport();        // テレポート関連の見た目（ディゾルブ・マーカー・グリッチ）を止める

	// 衝撃波リング（アリーナのギミック：中央のカバー箱から出る。同時に複数の輪を持てる）
	static constexpr float kShockwaveStartRadius = 2.0f;
	static constexpr float kShockwaveSpeed = 14.0f;    // 1秒あたりの広がる速さ[m/s]
	static constexpr float kShockwaveMaxRadius = 32.0f;
	static constexpr float kShockwaveHeight = 0.7f;
	static constexpr float kShockwaveHitHalfWidth = 0.4f;
	static constexpr float kShockwaveDamage = 20.0f;
	static constexpr float kShockwaveShakeDuration = 0.3f;
	static constexpr float kShockwaveShakeStrength = 0.02f;
	static constexpr int kMaxShockwaveRings = 4;           // 同時に出せる輪の数（P3は2箱同時＋突進の小さな輪）2→4（2026/11/11 突進の強化）
	struct ShockwaveRing {
		std::unique_ptr<Object3d> object;
		bool active = false;
		Vector3 center = { 0.0f, 0.0f, 0.0f };
		float radius = 0.0f;
		float maxRadius = kShockwaveMaxRadius; // この半径を超えたら消える（ギミックは大きく、突進は小さい）
		bool hasHit = false; // 1つの輪につき1回だけダメージ
	};
	std::array<ShockwaveRing, kMaxShockwaveRings> shockwaveRings;
	void StopShockwaves(); // 広がっている衝撃波を全部消す
	bool SpawnShockwaveRing(const Vector3& center, float maxRadius); // 空いている輪を1つ使って発生させる（空きがなければfalse）

	// 衝撃波ギミック（P2以降、一定間隔でランダムなカバー箱が予兆のあと衝撃波を出す）
	static constexpr float kShockwaveGimmickInterval = 12.0f;   // 発生間隔[秒]
	static constexpr float kShockwaveGimmickFirstDelay = 4.0f;  // P2に入ってから最初の発生までの待ち[秒]
	static constexpr float kShockwaveWarningDuration = 1.0f;    // 予兆（箱の点滅）の長さ[秒]
	static constexpr int kShockwaveWarningBlinkFrames = 4;      // 点滅の強弱を切り替えるフレーム数
	static constexpr float kShockwaveWarningIntensityLow = 0.4f;
	static constexpr float kShockwaveWarningIntensityHigh = 1.2f;
	static constexpr int kShockwaveBoxCountP2 = 1;              // 1回に出す箱の数（P2）
	static constexpr int kShockwaveBoxCountP3 = 2;              // 同（P3）
	static constexpr Vector3 kShockwaveWarningEmissive = { 1.0f, 0.35f, 0.05f }; // 予兆の発光色（橙赤）
	float shockwaveGimmickTimer = 0.0f;   // 0になったら予兆開始（P1では使わない）
	float shockwaveWarningTimer = 0.0f;   // 予兆の残り時間（0より大きければ予兆中）
	std::vector<size_t> shockwaveWarningWalls; // 予兆中の箱（wallsのインデックス）
	void UpdateShockwaveGimmick();        // タイマー進行・予兆・発生（ゲームが動いている間だけ呼ぶ）
	void StartShockwaveWarning();         // 箱を選んで予兆を始める
	void SpawnShockwaves();               // 予兆が終わった箱の足元から衝撃波を出す
	void CancelShockwaveWarning();        // 予兆を中断して箱の発光を戻す

	// --- 角のプラットフォーム崩壊（P2で2か所・P3で1か所。予兆→上から消える→消滅） ---
	// 消滅後のブロックはtranslateのyを画面外へ移し、当たり判定・レイキャスト・描画から実質外す
	enum class CornerState { kIntact, kWarning, kCollapsing, kGone };
	struct CornerData {
		int lowIndex = -1;   // 下段のwallsインデックス
		int highIndex = -1;  // 上段のwallsインデックス
		float x = 0.0f;
		float z = 0.0f;
		CornerState state = CornerState::kIntact;
		float timer = 0.0f;  // 現在の状態に入ってからの経過時間[秒]
	};
	static constexpr int kCornerCount = 4;
	static constexpr int kCornerCollapseCountP2 = 2;
	static constexpr int kCornerCollapseCountP3 = 1;
	static constexpr float kCornerWarningDuration = 1.5f;
	static constexpr float kCornerCollapseDuration = 0.8f;
	static constexpr int kCornerWarningBlinkFrames = 6;
	static constexpr float kCornerWarningIntensityLow = 0.4f;
	static constexpr float kCornerWarningIntensityHigh = 1.5f;
	static constexpr Vector3 kCornerWarningEmissive = { 1.0f, 0.1f, 0.1f };
	static constexpr float kCornerTopY = 5.0f;            // 上段の上端のワールドY
	static constexpr float kCornerHiddenY = -1000.0f;     // 消滅後に移す画面外のY
	static constexpr float kCornerClearMargin = 0.1f;     // ポータル・弾痕を消す範囲の広げ幅
	std::array<CornerData, kCornerCount> corners;
	int cornerCount = 0;
	std::vector<int> wallCornerIndex; // wallsと同じ並び。属する角の番号（なければ-1）
	void StartCornerCollapse(int count);     // kIntactの角からランダムにcount個をkWarningにする
	void UpdateCornerCollapse();             // 状態遷移と見た目の更新（ゲームが動いている間だけ呼ぶ）
	void FinishCornerGone(CornerData& corner); // 画面外へ移し、発光・ディゾルブを戻し、ポータル・弾痕を消す
	void SetCornerEmissive(const CornerData& corner, const Vector3& color, float intensity);
	void SetCornerDissolve(const CornerData& corner, float height, float edgeWidth, float time);
	void ResetCornerCollapse();              // 全て元の位置・見た目・状態に戻す（RestartGame用）
	void StopCornerCollapse();               // 決着時：予兆は取り消し、崩壊中は消滅まで確定させる
	bool IsWallShadowHidden(size_t wallIndex) const; // 崩壊中・消滅後は影を描かない

	// 遠距離グレネードの3連射（kRangedWindup終了後、kRangedBurst中に一定間隔で発射する）
	int bossGrenadeBurstRemaining = 0;
	float bossGrenadeBurstTimer = 0.0f;
	static constexpr int kBossGrenadeBurstCount = 3;
	static constexpr float kBossGrenadeBurstInterval = 0.2f; // 1発ごとの発射間隔

	// 近接突進がヒットした時のプレイヤーノックバック（ポータルのportalExitVelocityと同じ「減衰しながらmoveDirへ加算」方式）
	Vector3 knockbackVelocity = { 0.0f, 0.0f, 0.0f };
	float knockbackVelocityTimer = 0.0f;
	static constexpr float kKnockbackDuration = 0.3f;
	static constexpr float kKnockbackSpeed = 0.35f;
	// フェーズ移行の瞬間、ボスの近くにいたら吹き飛ばす（ダメージなし）
	static constexpr float kPhaseShiftKnockbackRadius = 8.0f;   // ボスからの水平距離がこれ未満なら対象
	static constexpr float kPhaseShiftKnockbackSpeed = 0.7f;    // 突進のノックバック（0.35）の2倍
	static constexpr float kPhaseShiftKnockbackUpSpeed = 0.3f;  // 上向きの跳ね上げ（ジャンプ0.38より少し弱め）

	// ボスの遠距離攻撃（グレネードランチャー的な範囲攻撃、着弾で爆発）
	struct Grenade {
		std::unique_ptr<Object3d> object;
		Vector3 position = {};
		Vector3 velocity = {};
		bool active = false;
	};
	std::vector<Grenade> grenades;
	// 同時に飛ばせる数。一斉発射（kBossRadialGrenadeCount）＋前の連射の残りが重なっても足りるようにする
	// （以前は3で、焦れたときの5連射の4・5発目が空き無しで撃たれていなかった）
	static const int kMaxGrenades = 16;
	// グレネード攻撃の2つの型：連続で吐き出す（今までの3連射。プレイヤーを狙う）／周りに一斉に吐き出す
	bool bossGrenadeRadial = false;                            // 今回の投擲が一斉発射か（予備動作の開始時に決める）
	static constexpr float kBossRadialGrenadeChance = 0.4f;    // 一斉発射を選ぶ確率（見えている・地上のときだけ）
	static constexpr int kBossRadialGrenadeCount = 8;          // 一斉発射の数
	static constexpr float kBossRadialGrenadeDistance = 9.0f;  // 一斉発射の着地点のボスからの距離
	static constexpr Vector3 kBossRangedWarningEmissive = { 1.0f, 0.5f, 0.05f };   // 連続：橙
	static constexpr Vector3 kBossRadialWarningEmissive = { 1.0f, 0.85f, 0.1f };   // 一斉：黄（予兆で見分けられるように）
	static constexpr float kGrenadeGravity = -0.02f;      // プレイヤーと同じ重力値を流用
	static constexpr float kGrenadeArcHeight = 5.0f;      // 山なり軌道の頂点の高さ（発射点からの相対高さ）
	static constexpr float kGrenadeAimSpread = 2.0f;      // 着弾狙い位置をプレイヤー中心にこの半径内でランダムにずらす（回避の余地＋3連射の着弾散らし）
	static constexpr float kGrenadeMinSpeed = 0.1f;       // 水平速度のクランプ下限（至近距離での過度なスロー化を防ぐ）
	static constexpr float kGrenadeMaxSpeed = 0.6f;       // 水平速度のクランプ上限（遠距離での過度な高速化を防ぐ）
	static constexpr float kGrenadeRadius = 0.3f;         // 床・壁との衝突判定半径
	static constexpr float kGrenadeExplosionRadius = 4.0f;
	static constexpr float kGrenadeDamage = 15.0f;
	// グレネードを発射する（非アクティブなものを1つ使う）。FireGrenade はプレイヤーの周りを狙う
	void FireGrenade();
	void FireRadialGrenades();                   // ボスの周りへ一斉に投げる
	void FireGrenadeAt(const Vector3& targetPos); // 指定した着地点へ山なりで投げる
	// グレネードを爆発させる（範囲内ならプレイヤーにダメージ、着弾演出を出す）
	void ExplodeGrenade(Grenade& grenade);

	// 弾痕デカール用
	std::unique_ptr<Model> planeModel = nullptr;
	// ポータル用モデル（ModelManagerのキャッシュ共有。PlacePortalで初回生成時にvisualへ設定する）
	// [組][0=a,1=b]
	std::array<std::array<std::shared_ptr<Model>, 2>, kMaxPortalPairs> portalModels;
	static const int kMaxBulletDecals = 32;
	std::vector<std::unique_ptr<Object3d>> bulletDecals;
	int nextDecalIndex = 0;

	// 視点武器（射撃モード＝ショットガン本体＋先台、ポータルガンモード＝ライフルのモデル）
	std::unique_ptr<Object3d> weaponBody = nullptr;
	std::unique_ptr<Object3d> weaponPump = nullptr; // 先台（発砲後に後ろへ引いて戻る）
	// ポータルガン。持ち替えが見た目で分かるよう別モデルにする（ショットガンと同じ原点・寸法で作ってある）
	std::unique_ptr<Object3d> weaponPortalGun = nullptr;
	Vector3 weaponBodyLocalOffset = { 0.22f, -0.20f, 0.45f };
	Vector3 weaponBarrelLocalOffset = { 0.22f, -0.18f, 0.62f }; // 銃口位置の基準（FireShotのmuzzleLocalとImGuiで使用。描画には使わない）
	float weaponRecoilTimer = 0.0f;
	float weaponPumpTimer = 0.0f;
	static constexpr float kPumpDuration = 0.4f;   // ポンプ動作全体の長さ(秒)
	static constexpr float kPumpStroke = 0.05f;    // 先台を引く量(m)
	static constexpr float kPumpPullStart = 0.1f;  // 発砲からこの秒数後に引き始める
	static constexpr float kPumpPullEnd = 0.25f;   // この秒数で引き切り、戻りに入る
	static constexpr float kRecoilDuration = 0.12f;   // リコイル演出の継続時間(秒)
	static constexpr float kRecoilKickAmount = 0.08f; // 発砲直後にカメラ側へ引く量(m)

	// 射撃間隔（ショットガン的に連射させず、1発ごとの重みを出すためのクールダウン）
	float fireCooldownTimer = 0.0f;
	static constexpr float kFireCooldownDuration = 0.5f; // 秒。この間はクリックしても発砲しない

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

	// プレイヤーHP
	float playerHP = kPlayerMaxHP;
	static constexpr float kPlayerMaxHP = 100.0f;

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

	// --- 武器表示（画面右下）：今の武器をカラーで手前に、もう一方をグレースケールで後ろ（左上にずらす）に出す ---
	// 画像は Tools/blender/weapon_icons.py で生成（銃口が右向き、背景透明、512x192）
	enum WeaponIcon { kIconShotgun, kIconShotgunGray, kIconPortalGun, kIconPortalGunGray, kWeaponIconCount };
	std::array<std::unique_ptr<Sprite>, kWeaponIconCount> weaponIcons;
	BitmapText weaponSlotTextFront; // 今の武器のキー番号
	BitmapText weaponSlotTextBack;  // もう一方の武器のキー番号
	void UpdateWeaponHud();
	// 位置は右下を基準（アンカー(1,1)）にする
	static constexpr Vector2 kWeaponIconFrontPos = { 1260.0f, 710.0f };
	static constexpr Vector2 kWeaponIconFrontSize = { 256.0f, 96.0f };
	static constexpr Vector2 kWeaponIconBackPos = { 1250.0f, 628.0f };
	static constexpr Vector2 kWeaponIconBackSize = { 176.0f, 66.0f };
	static constexpr Vector4 kWeaponIconBackColor = { 1.0f, 1.0f, 1.0f, 0.6f };
	// アイコン画像の大きさ（px）。クールタイムのゲージで左から切り抜くのに使う
	static constexpr Vector2 kWeaponIconTexSize = { 512.0f, 192.0f };

	// 武器の後ろのパネル（半透明の暗い板＋下端の細いライン）。アイコンが背景に溶けて色の変化が見えにくいため
	// 0=後ろの武器用、1=手前の武器用。位置は右下を基準（アンカー(1,1)）
	std::array<std::unique_ptr<Sprite>, 2> weaponPanels;
	std::array<std::unique_ptr<Sprite>, 2> weaponPanelLines;
	static constexpr Vector2 kWeaponPanelFrontPos = { 1272.0f, 716.0f };   // 右下の角
	static constexpr Vector2 kWeaponPanelFrontSize = { 302.0f, 112.0f };   // 番号まで含む
	static constexpr Vector2 kWeaponPanelBackPos = { 1258.0f, 632.0f };
	static constexpr Vector2 kWeaponPanelBackSize = { 210.0f, 78.0f };
	// アリーナ自体が暗い紺色なので、パネルは背景より明るいグレーがかった青にし、透け具合も控えめにする
	// （暗い色の半透明だと背景に溶けて見えなかった）。手前の方を明るくして後ろと区別する
	static constexpr Vector4 kWeaponPanelFrontColor = { 0.20f, 0.24f, 0.30f, 0.85f };
	static constexpr Vector4 kWeaponPanelBackColor = { 0.13f, 0.15f, 0.19f, 0.75f };
	static constexpr float kWeaponPanelLineHeight = 3.0f;
	static constexpr Vector4 kWeaponPanelFrontLineColor = { 0.0f, 0.75f, 1.0f, 0.9f };  // アリーナの発光と同じシアン
	static constexpr Vector4 kWeaponPanelBackLineColor = { 0.5f, 0.5f, 0.55f, 0.6f };

	// --- 効果音（Tools/sound/make_se.py で合成。名前＝ファイル名から .wav を除いたもの） ---
	static constexpr const char* kSoundDir = "Resources/sounds/";
	static constexpr const char* kSeShotgunFire = "shotgun_fire";
	static constexpr const char* kSeShotgunPump = "shotgun_pump";   // 先台の動きのタイミングに合わせた無音の間込み。発射と同時に鳴らす
	static constexpr const char* kSeHitWall = "hit_wall";
	static constexpr const char* kSeHitBoss = "hit_boss";
	static constexpr const char* kSeHitCore = "hit_core";
	static constexpr const char* kSeShieldBlock = "shield_block";
	static constexpr float kSeFireVolume = 1.0f;
	static constexpr float kSePumpVolume = 0.5f;
	static constexpr float kSeHitVolume = 0.6f;
	static constexpr float kSeHitWallVolume = 0.35f;
	void LoadSounds();
	void PlaySe(const char* name, float volume);

	// --- ポータルガンの操作ヒント（武器パネルの左隣。ポータルガンを持っている間だけ表示） ---
	// [マウス(左ボタン青)] [マウス(右ボタン橙)] [Rのキー] RESET
	std::unique_ptr<Sprite> portalHintPanel;
	std::array<std::unique_ptr<Sprite>, 2> portalHintMouseBodies;   // 0=左クリック用、1=右クリック用
	std::array<std::unique_ptr<Sprite>, 2> portalHintMouseButtons;  // 押すボタンをポータルの色で塗る
	std::unique_ptr<Sprite> portalHintKeyOuter;
	std::unique_ptr<Sprite> portalHintKeyInner;
	BitmapText portalHintKeyText;   // "R"
	BitmapText portalHintResetText; // "RESET"
	void InitializePortalHint();
	void UpdatePortalHint();
	void DrawPortalHint();
	static constexpr Vector2 kPortalHintOrigin = { 800.0f, 674.0f };  // パネルの左上（武器パネルの左端 970 より左）
	static constexpr Vector2 kPortalHintPanelSize = { 164.0f, 36.0f };
	static constexpr Vector2 kPortalHintMouseSize = { 18.0f, 28.0f };
	static constexpr Vector2 kPortalHintMouseButtonSize = { 9.0f, 12.0f };
	static constexpr std::array<float, 2> kPortalHintMouseOffsetX = { 8.0f, 32.0f };
	static constexpr float kPortalHintRowOffsetY = 4.0f;
	static constexpr Vector2 kPortalHintKeyOffset = { 60.0f, 5.0f };
	static constexpr float kPortalHintKeySize = 26.0f;
	static constexpr float kPortalHintKeyBorder = 2.0f;
	static constexpr float kPortalHintTextScale = 2.0f;
	static constexpr Vector2 kPortalHintKeyTextOffset = { 66.0f, 9.0f };
	static constexpr Vector2 kPortalHintResetTextOffset = { 92.0f, 9.0f };
	static constexpr Vector4 kPortalHintMouseColor = { 0.55f, 0.58f, 0.62f, 1.0f };
	// 置いた方のポータルのマウス（灰色。ボタンは本体より少し明るくして形は分かるようにする）
	static constexpr Vector4 kPortalHintUsedBodyColor = { 0.28f, 0.29f, 0.31f, 1.0f };
	static constexpr Vector4 kPortalHintUsedButtonColor = { 0.4f, 0.4f, 0.42f, 1.0f };
	static constexpr Vector4 kPortalHintKeyOuterColor = { 0.75f, 0.8f, 0.85f, 0.95f };
	static constexpr Vector4 kPortalHintKeyInnerColor = { 0.12f, 0.14f, 0.18f, 1.0f };
	static constexpr Vector4 kPortalAColor = { 0.1f, 0.45f, 1.0f, 1.0f };   // ポータルAのモデルの色に合わせる
	static constexpr Vector4 kPortalBColor = { 1.0f, 0.45f, 0.05f, 1.0f };  // ポータルBのモデルの色に合わせる
	static constexpr Vector4 kPortalCColor = { 0.2f, 1.0f, 0.35f, 1.0f };   // 2組目A（緑）
	static constexpr Vector4 kPortalDColor = { 1.0f, 0.3f, 0.75f, 1.0f };   // 2組目B（ピンク）
	static constexpr float kWeaponSlotTextFrontScale = 3.0f;
	static constexpr float kWeaponSlotTextBackScale = 2.0f;
	static constexpr Vector2 kWeaponSlotTextFrontPos = { kWeaponIconFrontPos.x - kWeaponIconFrontSize.x - 24.0f, kWeaponIconFrontPos.y - 60.0f };
	static constexpr Vector2 kWeaponSlotTextBackPos = { kWeaponIconBackPos.x - kWeaponIconBackSize.x - 18.0f, kWeaponIconBackPos.y - 44.0f };
	static constexpr Vector4 kWeaponSlotTextBackColor = { 0.6f, 0.6f, 0.6f, 0.8f };

	// ImGui非依存の設定メニュー（Pキーでのポーズ中のみ操作可能。Releaseビルドでもポストエフェクトを調整できる）
	PauseTuningMenu settingsMenu;

	std::unique_ptr<Sprite> bossHpBarBackground = nullptr;
	std::unique_ptr<Sprite> bossHpBarFill = nullptr;
	static constexpr float kBossHpBarMaxWidth = 500.0f;
	static constexpr float kBossHpBarHeight = 20.0f;
	static constexpr Vector2 kBossHpBarPosition = { 390.0f, 20.0f }; // 画面上部中央(1280幅基準)

	// 勝敗演出（全画面カラーオーバーレイのフェード＋中央にWIN/LOSE文字表示）
	bool gameOver = false;
	bool playerWon = false;
	float gameOverFadeTimer = 0.0f;
	static constexpr float kGameOverFadeDuration = 1.0f;
	static constexpr Vector3 kWinOverlayColor = { 0.75f, 0.6f, 0.15f };  // 勝利時の幕の色（金）
	static constexpr Vector3 kLoseOverlayColor = { 0.6f, 0.05f, 0.05f }; // 敗北時の幕の色（赤）
	static constexpr float kRetryFadeColorScale = 0.3f; // リトライ暗転は結果色をこの倍率で暗くして使う
	std::unique_ptr<Sprite> gameOverOverlay = nullptr;
	BitmapText gameOverText; // "WIN"/"LOSE"を中央表示（BitmapTextのグリフセル幅7*表示倍率2と一致させる複製定数がkGameOverTextCharWidth）
	static constexpr float kGameOverTextCharWidth = 14.0f;
	static constexpr float kGameOverTextPositionY = 300.0f;

	// 終了確認（ESCキーでいつでも開始。フェード無しの即時暗転＋"QUIT"表示。Enterで終了・ESCでキャンセル）
	bool quitConfirming = false;
	std::unique_ptr<Sprite> quitConfirmOverlay = nullptr;
	// ESCメニューの選択肢（↑↓/W・Sで選択、Enterで決定）
	enum QuitMenuItem { kQuitMenuTitle, kQuitMenuQuit, kQuitMenuCount };
	std::array<BitmapText, kQuitMenuCount> quitMenuTexts;
	int quitMenuIndex = kQuitMenuTitle;
	static constexpr float kQuitMenuLineSpacing = 30.0f; // 選択肢の行間[px]（文字の高さ9px×既定倍率2より広め）
	static constexpr Vector4 kQuitMenuSelectedColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr Vector4 kQuitMenuUnselectedColor = { 0.5f, 0.5f, 0.55f, 1.0f }; // TitleSceneの非選択色と揃える

	// 開始演出（READY中は移動・射撃・ボスAIを止め、FIGHT表示と同時に解禁）
	// フル版（初回開始）：黒帯＋ボスHPバーのチャージ＋READY中の視点ロック。短縮版（リトライ）：READY→FIGHTのみで視点操作可
	enum class IntroPhase { kNone, kReady, kFight };
	IntroPhase introPhase = IntroPhase::kNone;
	bool introIsFull = false;
	float introElapsed = 0.0f; // 現在のフェーズに入ってからの経過時間
	static constexpr float kIntroReadyDuration = 1.6f;     // 短縮版のREADY時間（シーン遷移の明転と重なる分を含む）
	static constexpr float kIntroFullReadyDuration = 2.2f; // フル版のREADY時間
	static constexpr float kIntroFullTextAppearTime = 1.0f; // フル版：黒帯が入ってからREADYの文字を出すまでの時間
	static constexpr float kIntroHpChargeStart = 0.8f;     // フル版：ボスHPバーのチャージ開始時刻
	static constexpr float kIntroHpChargeEnd = 1.8f;       // フル版：ボスHPバーのチャージ完了時刻
	static constexpr float kIntroBarHeight = 80.0f;        // 黒帯1本の高さ
	static constexpr float kIntroBarSlideDuration = 0.4f;  // 黒帯が入ってくる時間
	static constexpr float kIntroBarRetractDuration = 0.4f; // READY終盤に黒帯を引く時間（FIGHTの瞬間には消え切っている）
	static constexpr float kIntroFightDuration = 0.6f;     // FIGHT表示時間（この間は操作可能、徐々に消える）
	// フル版：ボスの実体化。シーン遷移の明転（0.6秒）中はSceneTransitionがグリッチ強度を毎フレーム上書きするため、明転後に始める
	static constexpr float kIntroMaterializeStart = 0.6f;
	static constexpr float kIntroMaterializeDuration = 1.0f; // 開始0.6s+1.0s=1.6sで完了し、黒帯の引き始め(2.2-0.4=1.8s)より前に収まる
	static constexpr float kIntroMaterializeFlickerEnd = 0.5f; // 実体化の進行率がここまではちらつかせる
	static constexpr float kIntroMaterializeEdgeWidth = 0.6f;  // 実体化の境界の発光帯の幅[m]
	static constexpr float kIntroMaterializeBlockFadeRatio = 0.3f; // ブロック欠けを弱め始める位置（境界が残り身長のこの割合に来たら弱め、頭頂で0）
	static constexpr float kIntroMaterializeGlitch = 0.7f;     // 実体化開始時のグリッチ強度（実体化完了に向けて0へ）
	static constexpr float kIntroFightGlitch = 1.0f;           // FIGHTの瞬間のグリッチ強度（短縮版でも出す）
	static constexpr float kIntroFightGlitchDuration = 0.25f;
	// フル版：演出用カメラ。プレイヤーとボスの間に中央カバー箱(0,*,12)があり実体化が隠れるため、
	// 箱を避けたボスの右斜め前の低い位置から見上げ、READY中に少しずつ寄る
	static constexpr Vector3 kIntroCameraStartPos = { 9.2f, 1.2f, 7.5f };  // ボスまで約15.5m
	static constexpr Vector3 kIntroCameraEndPos = { 7.0f, 1.6f, 10.2f };   // ボスまで約12m
	static constexpr float kIntroCameraLookHeight = 3.0f; // ボスの足元からこの高さを注視する（見上げる構図）
	Vector3 introSavedCameraRotate = {}; // 演出前のプレイヤーの視点（演出後にここへ戻す）
	std::unique_ptr<Sprite> introBarTop = nullptr;
	std::unique_ptr<Sprite> introBarBottom = nullptr;
	float bossHpBarIntroScale = 1.0f; // ボスHPバーの表示倍率（チャージ演出用、実際のbossHPには影響しない）
	static constexpr float kIntroTextScale = 5.0f;
	static constexpr float kIntroTextPositionY = 320.0f;
	static constexpr Vector4 kIntroReadyColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	static constexpr Vector4 kIntroFightColor = { 0.3f, 1.0f, 1.0f, 1.0f }; // タイル遷移と揃えたシアン
	static constexpr float kIntroFightShakeDuration = 0.2f;
	static constexpr float kIntroFightShakeStrength = 0.015f;
	BitmapText introText;

	ScreenEffectController* screenEffect = nullptr; // MyGameが所有する画面の効果の窓口（非所有）
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
	std::array<PortalPair, kMaxPortalPairs> portalPairs; // 1組目:青/オレンジ、2組目:緑/ピンク
	static constexpr float kPortalRadius = 1.0f;
	// ワープで出てきたポータル。一度その範囲（kPortalRadius＋余裕）から出るまで、入口として反応させない
	const Portal* portalExitLock = nullptr;
	static constexpr float kPortalExitLockMargin = 0.2f;
	// 2つのポータルの間の距離の上限。もう一方が置いてあるとき、これより離れた場所には置けない
	static constexpr float kPortalMaxPairDistance = 25.0f;
	// 置けなかったときに、レティクルを一瞬この色にして知らせる
	float portalRejectTimer = 0.0f;
	static constexpr float kPortalRejectFlashDuration = 0.25f;
	static constexpr Vector4 kReticleColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	static constexpr Vector4 kReticleRejectColor = { 0.45f, 0.45f, 0.45f, 1.0f };
	static constexpr float kPortalTeleportCooldown = 0.3f;
	float portalCooldownTimer = 0.0f;

	// テレポート直後の勢い継承（移動方向に一定時間だけ加算し、徐々に減衰させる）
	Vector3 portalExitVelocity = { 0, 0, 0 };
	float portalExitVelocityTimer = 0.0f;
	static constexpr float kPortalExitVelocityDuration = 0.25f;
};
