# ボスのモデルを生成する（本体・コア・外れる装甲4パーツ）
#
# 実行: blender -b --factory-startup -P Tools/blender/boss.py -- Resources/models/boss <プレビュー出力先>
#
# 大きさ・原点（今の当たり判定にそのまま収まるようにしている）:
#   全体が 幅4 × 高さ5 × 奥行4 の箱（ArenaLayout::kBossScale {2,2.5,2} の half-extent）の中に収まる。
#   原点 = その箱の中心（底面が -2.5、頭頂が +2.5）。ゲーム側ではスケール {1,1,1} で、
#   今と同じ位置（bossPos.y + kBossHalfHeight）に置く。
#   正面 = Blender の +Y = エンジンの +Z（今の SetRotate の atan2(dir.x, dir.z) と同じ向き）。
#
# パーツは全部同じ原点で書き出しているので、ゲーム側では全パーツに同じ位置・回転を与えれば組み上がる。
#   boss_body         … 本体（土台・胴・頭・腕）。常に表示
#   boss_core         … 弱点のコア。胴の上・頭の後ろ。P2 で上の装甲が外れると上から見える
#   boss_armor_back   … コアを覆う上部の装甲（P2 で外れる）
#   boss_armor_chest  … 胸の装甲
#   boss_armor_shoulder_l / _r … 肩の装甲
#
# 発光ラインはマテリアル "Boss_Glow" に分けてあるので、P3 の赤化や攻撃の予兆はコードで発光色を変えて表現できる。

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

SLICE_STEP = 0.15  # 実体化演出の横ズレ用に、高さ方向をこの間隔で輪切りにする

BODY, TRIM, GLOW = 0, 1, 2
GLOW_COLOR = (0.75, 0.15, 1.0)  # 紫（今のボスの色 {0.5,0,0.4} の系統。プレイヤー側のシアンと区別する）
GLOW_STRENGTH = 4.0
CORE_COLOR = (1.0, 0.3, 0.75)   # コアはピンク寄りで一段強く光らせる
CORE_STRENGTH = 8.0

# コアの位置（胴の上面 z=1.2 に半分埋まり、頭の後ろにある）
CORE_CENTER = (0.0, -0.55, 1.5)


def make_materials():
    return [
        c.make_material("Boss_Body", (0.07, 0.06, 0.09), metallic=0.5, roughness=0.5),
        c.make_material("Boss_Trim", (0.18, 0.16, 0.22), metallic=0.4, roughness=0.6),
        c.make_material("Boss_Glow", GLOW_COLOR, emission=GLOW_COLOR, emission_strength=GLOW_STRENGTH),
    ]


def build_body(mats):
    b = c.MeshBuilder()
    # --- 土台（浮遊ユニット）と発光リング ---
    base = c.ngon_profile(0.0, 0.0, 1.7, 8, rot=0.3927)
    ring = c.ngon_profile(0.0, 0.0, 1.76, 8, rot=0.3927)
    _vertical_prism(b, base, -2.5, -1.7, BODY)
    _vertical_prism(b, ring, -2.05, -1.95, GLOW)
    _vertical_prism(b, c.ngon_profile(0.0, 0.0, 1.2, 8, rot=0.3927), -1.7, -1.55, TRIM)  # 腰

    # --- 胴 ---
    b.prism_y(c.chamfer_rect_profile(-1.3, 1.3, -1.55, 1.2, 0.25), -1.15, 1.1, BODY)
    # 胸の装甲が外れたときに見える、胴の正面の発光ライン
    b.box((-0.75, 1.1, -1.2), (-0.62, 1.13, 0.9), GLOW)
    b.box((0.62, 1.1, -1.2), (0.75, 1.13, 0.9), GLOW)
    b.box((-0.4, 1.1, -0.3), (0.4, 1.13, -0.2), GLOW)
    # 胴の側面の発光ライン
    b.box_mirror_x((1.3, -0.9, -0.6), (1.33, 0.9, -0.5), GLOW)

    # --- 頭（正面にバイザー） ---
    b.prism_y(c.chamfer_rect_profile(-0.55, 0.55, 1.2, 1.95, 0.15), 0.05, 0.95, TRIM)
    b.box((-0.42, 0.95, 1.5), (0.42, 0.98, 1.64), GLOW)

    # --- 腕（前腕のキャノン。先端に発光） ---
    for sign in (1.0, -1.0):
        x0, x1 = sorted((sign * 1.35, sign * 1.95))
        b.prism_y(c.chamfer_rect_profile(x0, x1, -1.4, 0.3, 0.12), -0.6, 0.9, BODY)
        cx = (x0 + x1) * 0.5
        b.prism_y(c.ngon_profile(cx, -0.9, 0.2, 6, rot=0.5236), 0.9, 1.15, TRIM)   # 砲口
        b.prism_y(c.ngon_profile(cx, -0.9, 0.13, 6, rot=0.5236), 1.15, 1.17, GLOW)  # 砲口の発光
    # 腕の外側の発光ライン（左右対称）
    b.box_mirror_x((1.95, -0.4, -0.25), (1.98, 0.7, -0.15), GLOW)
    return b.finish("boss_body", mats, slice_step=SLICE_STEP)


def _vertical_prism(b, profile_xy, z0, z1, mat_index):
    """XY 平面の輪郭を Z 方向に押し出す（土台など、上下に伸びる柱）"""
    bm = b.bm
    low = [bm.verts.new((x, y, z0)) for x, y in profile_xy]
    high = [bm.verts.new((x, y, z1)) for x, y in profile_xy]
    faces = [bm.faces.new(high), bm.faces.new(list(reversed(low)))]
    n = len(profile_xy)
    for i in range(n):
        j = (i + 1) % n
        faces.append(bm.faces.new((low[i], low[j], high[j], high[i])))
    for f in faces:
        f.material_index = mat_index


def build_core(mats):
    b = c.MeshBuilder()
    core_mat = c.make_material("Boss_Core", CORE_COLOR, emission=CORE_COLOR, emission_strength=CORE_STRENGTH)
    c.octahedron(b, CORE_CENTER, 0.5, 0.55, 0)
    return b.finish("boss_core", [core_mat], slice_step=SLICE_STEP)


def build_armor_back(mats):
    b = c.MeshBuilder()
    # コアを上と後ろから覆うカバー。上面は斜めに落とす
    b.prism_y(c.chamfer_rect_profile(-0.85, 0.85, 1.15, 2.3, 0.3), -1.2, 0.0, TRIM)
    b.box((-0.5, -1.23, 1.5), (0.5, -1.2, 1.6), GLOW)          # 背面の発光ライン
    b.box((-0.06, -1.1, 2.3), (0.06, -0.1, 2.33), GLOW)         # 上面の発光ライン（上から見える）
    return b.finish("boss_armor_back", mats, slice_step=SLICE_STEP)


def build_armor_chest(mats):
    b = c.MeshBuilder()
    b.prism_y(c.chamfer_rect_profile(-1.05, 1.05, -1.1, 0.95, 0.2), 1.1, 1.35, TRIM)
    b.box((-0.75, 1.35, 0.55), (0.75, 1.38, 0.65), GLOW)  # 胸の横ライン
    b.box((-0.06, 1.35, -0.9), (0.06, 1.38, 0.5), GLOW)   # 胸の縦ライン
    return b.finish("boss_armor_chest", mats, slice_step=SLICE_STEP)


def build_armor_shoulder(mats, sign):
    b = c.MeshBuilder()
    x0, x1 = sorted((sign * 1.2, sign * 2.0))
    b.prism_y(c.chamfer_rect_profile(x0, x1, 0.3, 1.45, 0.25), -0.75, 0.75, TRIM)
    gx0, gx1 = sorted((sign * 1.5, sign * 1.7))
    b.box((gx0, 0.75, 0.75), (gx1, 0.78, 1.1), GLOW)  # 肩の正面の発光
    name = "boss_armor_shoulder_r" if sign > 0 else "boss_armor_shoulder_l"
    return b.finish(name, mats, slice_step=SLICE_STEP)


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "boss"))

    c.reset_scene()
    mats = make_materials()
    parts = {
        "boss_body": build_body(mats),
        "boss_core": build_core(mats),
        "boss_armor_back": build_armor_back(mats),
        "boss_armor_chest": build_armor_chest(mats),
        "boss_armor_shoulder_l": build_armor_shoulder(mats, -1.0),
        "boss_armor_shoulder_r": build_armor_shoulder(mats, 1.0),
    }
    c.save_blend(out_dir, "boss")
    for name, obj in parts.items():
        c.export_glb(obj, out_dir, name)

    c.setup_preview_scene()
    # P1（全装甲あり）：斜め前と、プレイヤーの目線（スポーン位置から見上げる）
    c.render_view("Front", (7.0, 9.0, 2.5), (0.0, 0.0, 0.0), os.path.join(preview_dir, "boss_p1_front.png"))
    c.render_view("Back", (-6.0, -8.0, 4.0), (0.0, 0.0, 0.5), os.path.join(preview_dir, "boss_p1_back.png"))
    # P2（装甲が外れた状態）：データ足場から見下ろす想定で、上からコアが見えるか確認
    for name in ("boss_armor_back", "boss_armor_chest", "boss_armor_shoulder_l", "boss_armor_shoulder_r"):
        parts[name].hide_render = True
    c.render_view("P2Top", (4.0, 6.0, 9.0), (0.0, 0.0, 1.0), os.path.join(preview_dir, "boss_p2_above.png"))
    c.render_view("P2Front", (7.0, 9.0, 2.5), (0.0, 0.0, 0.0), os.path.join(preview_dir, "boss_p2_front.png"))


main()
