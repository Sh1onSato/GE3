#pragma once
#include "Structs.h"
#include <array>

/// <summary>
/// アリーナ（床・壁・ボス）の配置データ。GameSceneとTitleScene（3D背景）の両方から参照し、
/// 配置を2か所に複製してズレるのを防ぐ。
/// </summary>
namespace ArenaLayout {

// 壁の種類（種類ごとに見た目のモデルを変える。当たり判定は種類によらず pos/scale の箱）
enum class WallKind { kOuterZ, kOuterX, kCover, kPlatformLow, kPlatformHigh };

struct WallData {
	Vector3 pos;
	Vector3 scale;
	WallKind kind;
};

// --- 床 ---
inline constexpr Vector3 kFloorScale = { 30.0f, 1.0f, 30.0f };
inline constexpr Vector3 kFloorTranslate = { 0.0f, -1.0f, 0.0f };
inline constexpr Vector4 kFloorColor = { 0.5f, 0.5f, 0.5f, 1.0f };

// --- 壁（外壁4・中央カバー4・四隅プラットフォーム8） ---
inline constexpr std::array<WallData, 16> kWallDatas = { {
	// --- 外壁（閉じた箱型アリーナの四方。±30に拡張し、1v1ボス戦を見据えた構成） ---
	// 上下移動(四隅の上段プラットフォーム高さ5.0＋ジャンプ最大加算高さ約3.61=最大到達約8.6)を考慮し、
	// 場外に出られないよう上端をy=15.0まで引き上げ（下端y=-1.0は床への埋め込みのため変更なし）
	{ { 0.0f, 7.0f, 30.0f }, { 30.0f, 8.0f, 1.0f }, WallKind::kOuterZ },
	{ { 0.0f, 7.0f, -30.0f }, { 30.0f, 8.0f, 1.0f }, WallKind::kOuterZ },
	{ { 30.0f, 7.0f, 0.0f }, { 1.0f, 8.0f, 30.0f }, WallKind::kOuterX },
	{ { -30.0f, 7.0f, 0.0f }, { 1.0f, 8.0f, 30.0f }, WallKind::kOuterX },
	// --- 中央のカバー（十字型に対称配置。中心は空けて見通しを確保しつつ、物陰に隠れられるようにする） ---
	{ { 12.0f, 1.0f, 0.0f }, { 1.5f, 2.0f, 1.5f }, WallKind::kCover },
	{ { -12.0f, 1.0f, 0.0f }, { 1.5f, 2.0f, 1.5f }, WallKind::kCover },
	{ { 0.0f, 1.0f, 12.0f }, { 1.5f, 2.0f, 1.5f }, WallKind::kCover },
	{ { 0.0f, 1.0f, -12.0f }, { 1.5f, 2.0f, 1.5f }, WallKind::kCover },
	// --- 四隅の2段プラットフォーム（上下を使った戦い用。ジャンプ1回で下段(高さ2.5)、
	//     さらにもう1回ジャンプで上段(高さ5.0)に登れる高低差にしている） ---
	{ { 14.0f, 1.25f, 14.0f }, { 3.0f, 1.25f, 3.0f }, WallKind::kPlatformLow },
	{ { 14.0f, 3.75f, 14.0f }, { 1.5f, 1.25f, 1.5f }, WallKind::kPlatformHigh },
	{ { -14.0f, 1.25f, 14.0f }, { 3.0f, 1.25f, 3.0f }, WallKind::kPlatformLow },
	{ { -14.0f, 3.75f, 14.0f }, { 1.5f, 1.25f, 1.5f }, WallKind::kPlatformHigh },
	{ { 14.0f, 1.25f, -14.0f }, { 3.0f, 1.25f, 3.0f }, WallKind::kPlatformLow },
	{ { 14.0f, 3.75f, -14.0f }, { 1.5f, 1.25f, 1.5f }, WallKind::kPlatformHigh },
	{ { -14.0f, 1.25f, -14.0f }, { 3.0f, 1.25f, 3.0f }, WallKind::kPlatformLow },
	{ { -14.0f, 3.75f, -14.0f }, { 1.5f, 1.25f, 1.5f }, WallKind::kPlatformHigh },
} };

// --- アリーナ用モデル（GameSceneとTitleSceneで共有） ---
inline constexpr const char* kArenaModelDir = "Resources/models/arena";
inline constexpr const char* kFloorModelFile = "floor.glb";

inline const char* WallModelFile(WallKind kind) {
	switch (kind) {
	case WallKind::kOuterZ: return "wall_z.glb";
	case WallKind::kOuterX: return "wall_x.glb";
	case WallKind::kCover: return "cover.glb";
	case WallKind::kPlatformLow: return "platform_low.glb";
	case WallKind::kPlatformHigh: return "platform_high.glb";
	}
	return "cover.glb";
}

// --- ボス ---
// 中央カバー(±12)・角プラットフォーム(±14)と被らない開けた位置。足元基準（playerPosと同じ規約）
inline constexpr Vector3 kBossInitialPos = { 0.0f, 0.0f, 20.0f };
// 底面を床(y=0)に合わせるための高さオフセット（kBossScale.y の半分）
inline constexpr float kBossHalfHeight = 2.5f;
inline constexpr Vector3 kBossScale = { 2.0f, 2.5f, 2.0f };
inline constexpr Vector4 kBossColor = { 0.5f, 0.0f, 0.4f, 1.0f }; // 紫系でプレイヤー・的と区別する

// ボスの見た目は5パーツ（X字に割れたくさび4つ＋コア）。順序はGameScene/TitleSceneのパーツ配列と一致させる
inline constexpr const char* kBossModelDir = "Resources/models/boss_cube";
inline constexpr std::array<const char*, 5> kBossPartFiles = {
	"boss_cube_wedge_top.glb", "boss_cube_wedge_bottom.glb", "boss_cube_wedge_left.glb", "boss_cube_wedge_right.glb", "boss_cube_core.glb",
};

} // namespace ArenaLayout
