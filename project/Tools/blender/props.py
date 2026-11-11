# 小物のモデルを生成する（グレネード・ポータルの枠・的・回復キューブ・レーザー・衝撃波）
#
# 実行: blender -b --factory-startup -P Tools/blender/props.py -- Resources/models/props <プレビュー出力先>
#
# 大きさの決まり（arena.py と同じく、今の SetScale をそのまま使えるようにしている）:
#   grenade      … 今の scale {0.2,0.2,0.2} で使う（実寸 半径0.2m）。危険物なのでオレンジ
#   portal_a/_b  … 今の planeModel と同じ決まり：-1〜1 の正方形で、正面（法線）= エンジンの +Z。
#                  今の SetScale（kPortalHalfSize=1.5、端で軸ごとに縮む）と DirectionToDirection({0,0,1}, normal) のまま使える。
#                  A=青、B=オレンジ（発光色を1つのモデルで切り替えるより、2つに分けたほうが確実なため）
#   target       … 今の的の scale {0.5,0.75,0.5} で使う（実寸 1×1.5×1m）
#   heal_cube    … 新規。scale {1,1,1} で実寸 0.6m 角（コードで回転させる想定）
#   laser        … 新規。原点 = 発射口、エンジンの +Z 方向へ長さ 1。断面の半径 1。
#                  コードで scale {太さ, 太さ, 長さ} にして、ボスの向きに回して使う
#   shockwave    … 新規。原点 = 床の上の中心、半径 1・高さ 1 の輪。
#                  コードで scale {半径, 高さ, 半径} にして広げる（輪の幅は半径に比例して太る）

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

BODY, TRIM, GLOW = 0, 1, 2
T = 0.012  # 発光ラインの出っ張り（実寸）

PORTAL_A = (0.1, 0.45, 1.0)
PORTAL_B = (1.0, 0.45, 0.05)
DANGER = (1.0, 0.35, 0.05)      # グレネード
TARGET = (1.0, 0.55, 0.1)       # 的（今のオレンジ色の的に合わせる）
HEAL = (0.3, 1.0, 0.45)         # 回復
BOSS_ATTACK = (0.85, 0.2, 1.0)  # ボスの攻撃（ボスの発光色に合わせた紫）
SHIELD = (0.3, 0.7, 1.0)        # ボスのシールド（攻撃の紫と見分ける青）


def mats3(prefix, glow, strength, body=(0.06, 0.06, 0.08), trim=(0.16, 0.16, 0.2)):
    return [
        c.make_material(prefix + "_Body", body, metallic=0.4, roughness=0.5),
        c.make_material(prefix + "_Trim", trim, metallic=0.4, roughness=0.6),
        c.make_material(prefix + "_Glow", glow, emission=glow, emission_strength=strength),
    ]


def glow_only(prefix, glow, strength):
    return [c.make_material(prefix + "_Glow", glow, emission=glow, emission_strength=strength)]


# ---------------------------------------------------------------------------
def build_grenade():
    """角ばった爆弾。上下にとがった8角柱＋赤道の発光リング＋上下の発光キャップ"""
    m = mats3("Grenade", DANGER, 5.0)
    b = c.MeshBuilder()
    r = 0.2
    b.prism_z(c.ngon_profile(0, 0, r, 8, rot=0.3927), -0.1, 0.1, BODY)
    b.prism_z(c.ngon_profile(0, 0, r * 0.6, 8, rot=0.3927), 0.1, 0.17, TRIM)
    b.prism_z(c.ngon_profile(0, 0, r * 0.6, 8, rot=0.3927), -0.17, -0.1, TRIM)
    b.prism_z(c.ngon_profile(0, 0, r * 0.35, 8, rot=0.3927), 0.17, 0.2, GLOW)
    b.prism_z(c.ngon_profile(0, 0, r * 0.35, 8, rot=0.3927), -0.2, -0.17, GLOW)
    b.prism_z(c.ngon_profile(0, 0, r + 0.01, 8, rot=0.3927), -0.025, 0.025, GLOW)
    return b.finish("grenade", m, engine_scale=(0.2, 0.2, 0.2))


def build_portal(name, color):
    """8角形の枠。-1〜1 の正方形に収め、正面（法線）は Blender の +Y（= エンジンの +Z）。
    奥行きはごく薄くし、壁から少しだけ手前に出す（今の配置は当たった点＋法線方向へ少し浮かせている）"""
    m = [
        c.make_material(name + "_Frame", (0.05, 0.05, 0.07), metallic=0.4, roughness=0.5),
        c.make_material(name + "_Membrane", tuple(v * 0.25 for v in color), emission=color, emission_strength=0.6),
        c.make_material(name + "_Glow", color, emission=color, emission_strength=5.0),
    ]
    FRAME, MEMBRANE, RING = 0, 1, 2
    b = c.MeshBuilder()
    rot = 0.3927
    # 半径は「辺までの距離」で指定する。外枠の辺が ±1 に接する（-1〜1 の正方形にぴったり内接する）ようにして、
    # 以前の正方形の板と同じ大きさに見せる（頂点は (1, 0.414) などで、正方形からはみ出さない）
    k = 1.0 / math.cos(math.pi / 8)  # 辺までの距離 → 頂点までの半径
    outer = c.ngon_profile(0, 0, 1.0 * k, 8, rot=rot)
    mid = c.ngon_profile(0, 0, 0.9 * k, 8, rot=rot)
    inner = c.ngon_profile(0, 0, 0.83 * k, 8, rot=rot)
    _ring_y(b, outer, mid, 0.0, 0.03, FRAME)      # 外枠
    _ring_y(b, mid, inner, 0.0, 0.04, RING)       # 発光する内枠（外枠より少し手前）
    b.prism_y(inner, 0.0, 0.01, MEMBRANE)         # 中の膜（暗めに光る）
    b.prism_y(c.ngon_profile(0, 0, 0.47 * k, 8, rot=rot), 0.01, 0.015, RING)  # 中心の小さな光
    b.prism_y(c.ngon_profile(0, 0, 0.42 * k, 8, rot=rot), 0.015, 0.016, MEMBRANE)
    # 外枠の4か所の目印（上下左右）。外枠の辺（1.0）の内側にかかる位置
    for ang in (0, 90, 180, 270):
        mat = c.Matrix.Rotation(math.radians(ang), 4, "Y")
        b.box((-0.06, 0.0, 0.88), (0.06, 0.045, 0.99), RING, matrix=mat)
    return b.finish(name, m)


def _ring_y(b, outer, inner, y0, y1, mat_index):
    """XZ 平面の2つの輪郭の間（輪）を Y 方向に押し出す"""
    n = len(outer)
    for i in range(n):
        j = (i + 1) % n
        quad = [outer[i], outer[j], inner[j], inner[i]]
        b.prism_y(quad, y0, y1, mat_index)


def build_target():
    """的。面取りした柱の4面に、発光する同心の四角（ブルズアイ）"""
    m = mats3("Target", TARGET, 4.0)
    b = c.MeshBuilder()
    hx, hz = 0.5, 0.75
    b.prism_z(c.chamfer_rect_profile(-hx, hx, -hx, hx, 0.06), -hz, hz, BODY)
    for ang in (0, 90, 180, 270):
        rot = c.Matrix.Rotation(math.radians(ang), 4, "Z")
        for s, w in ((0.32, 0.03), (0.18, 0.03)):
            # 正面（+Y 面）に四角い輪を描き、回して4面に配る
            b.box((-s, hx, 0.15 - s), (s, hx + T, 0.15 - s + w), GLOW, matrix=rot)
            b.box((-s, hx, 0.15 + s - w), (s, hx + T, 0.15 + s), GLOW, matrix=rot)
            b.box((-s, hx, 0.15 - s), (-s + w, hx + T, 0.15 + s), GLOW, matrix=rot)
            b.box((s - w, hx, 0.15 - s), (s, hx + T, 0.15 + s), GLOW, matrix=rot)
        b.box((-0.05, hx, 0.1), (0.05, hx + T, 0.2), GLOW, matrix=rot)  # 中心
    b.band_around(hx, hx, hz - 0.1, hz - 0.05, T, GLOW, inset=0.06)
    return b.finish("target", m, engine_scale=(0.5, 0.75, 0.5))


def build_heal_cube():
    """回復キューブ。12本の辺だけの枠＋中に浮かぶ十字（緑）"""
    m = mats3("Heal", HEAL, 4.0, body=(0.05, 0.1, 0.07))
    b = c.MeshBuilder()
    h, w = 0.3, 0.035
    for sx in (-1, 1):
        for sy in (-1, 1):
            b.box((sx * h - w, sy * h - w, -h), (sx * h + w, sy * h + w, h), GLOW)   # 縦の辺
            b.box((-h, sy * h - w, sx * h - w), (h, sy * h + w, sx * h + w), GLOW)   # X 方向の辺
            b.box((sx * h - w, -h, sy * h - w), (sx * h + w, h, sy * h + w), GLOW)   # Y 方向の辺
    a, t = 0.16, 0.05  # 中の十字（3軸）
    b.box((-a, -t, -t), (a, t, t), BODY)
    b.box((-t, -a, -t), (t, a, t), BODY)
    b.box((-t, -t, -a), (t, t, a), BODY)
    b.box((-0.07, -0.07, -0.07), (0.07, 0.07, 0.07), GLOW)
    return b.finish("heal_cube", m)


def build_boss_shield():
    """ボスが正面に張るシールド。-1〜1 の正方形、正面（法線）= Blender の +Y（エンジンの +Z）。
    中を塞ぐ膜は付けず、発光する格子だけにして、向こうのボスが透けて見えるようにする。
    コードで scale {横の半分, 縦の半分, 1} にして、ボスの正面に置く"""
    m = glow_only("Shield", SHIELD, 4.0)
    b = c.MeshBuilder()
    d = 0.03  # 奥行き
    w = 0.05  # 外枠の太さ
    # 外枠（角を落とした8角形の枠を、正方形に近い形で）
    rot = 0.3927
    k = 1.0 / math.cos(math.pi / 8)
    outer = c.ngon_profile(0, 0, 1.0 * k, 8, rot=rot)
    inner = c.ngon_profile(0, 0, (1.0 - w) * k, 8, rot=rot)
    _ring_y(b, outer, inner, 0.0, d, 0)
    # 内側の格子（縦横5本ずつ。8角形の中に収まる長さにする）
    for i in range(-2, 3):
        p = i * 0.38
        half = (1.0 - w) - max(0.0, abs(p) - 0.4) * 1.0  # 角の斜めの辺で短くする
        b.box((p - 0.012, 0.0, -half), (p + 0.012, d * 0.6, half), 0)
        b.box((-half, 0.0, p - 0.012), (half, d * 0.6, p + 0.012), 0)
    return b.finish("boss_shield", m)


def build_laser():
    """ボスのレーザー。原点から +Y（エンジンの +Z）へ長さ 1、断面は半径 1 の6角形"""
    m = glow_only("Laser", BOSS_ATTACK, 8.0)
    b = c.MeshBuilder()
    b.prism_y(c.ngon_profile(0, 0, 1.0, 6, rot=0.5236), 0.0, 1.0, 0)
    return b.finish("laser", m)


def build_shockwave():
    """衝撃波の輪。原点 = 床の上の中心、外半径 1・内半径 0.9・高さ 1。上端に向かって細くなる"""
    m = glow_only("Shockwave", BOSS_ATTACK, 5.0)
    b = c.MeshBuilder()
    n = 32
    bm = b.bm
    rings = []
    for z, r_out, r_in in ((0.0, 1.0, 0.9), (1.0, 0.98, 0.94)):
        outer = [bm.verts.new((r_out * math.cos(2 * math.pi * i / n), r_out * math.sin(2 * math.pi * i / n), z))
                 for i in range(n)]
        inner = [bm.verts.new((r_in * math.cos(2 * math.pi * i / n), r_in * math.sin(2 * math.pi * i / n), z))
                 for i in range(n)]
        rings.append((outer, inner))
    (o0, i0), (o1, i1) = rings
    for k in range(n):
        j = (k + 1) % n
        for quad in ((o0[k], o0[j], o1[j], o1[k]),      # 外側の面
                     (i0[j], i0[k], i1[k], i1[j]),      # 内側の面
                     (o1[k], o1[j], i1[j], i1[k]),      # 上面
                     (o0[j], o0[k], i0[k], i0[j])):     # 底面
            bm.faces.new(quad).material_index = 0
    return b.finish("shockwave", m)


# ---------------------------------------------------------------------------
def place(obj, loc_blender, scale_engine, rot_deg=(0, 0, 0)):
    inst = c.bpy.data.objects.new(obj.name + "_inst", obj.data)
    inst.location = loc_blender
    inst.scale = (scale_engine[0], scale_engine[2], scale_engine[1])
    inst.rotation_euler = tuple(math.radians(a) for a in rot_deg)
    c.bpy.context.scene.collection.objects.link(inst)


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "props"))

    c.reset_scene()
    parts = {
        "grenade": build_grenade(),
        "portal_a": build_portal("portal_a", PORTAL_A),
        "portal_b": build_portal("portal_b", PORTAL_B),
        "target": build_target(),
        "heal_cube": build_heal_cube(),
        "laser": build_laser(),
        "shockwave": build_shockwave(),
        "boss_shield": build_boss_shield(),
    }
    c.save_blend(out_dir, "props")
    for name, obj in parts.items():
        c.export_glb(obj, out_dir, name)

    # プレビュー：ゲームで使う大きさに拡大して並べる（左から順に）
    c.setup_preview_scene()
    for obj in parts.values():
        obj.hide_render = True
    place(parts["grenade"], (-6.0, 0.0, 0.4), (0.2, 0.2, 0.2))
    place(parts["target"], (-4.2, 0.0, 0.75), (0.5, 0.75, 0.5))
    place(parts["portal_a"], (-1.8, 0.0, 1.6), (1.5, 1.5, 1.5))
    place(parts["portal_b"], (1.6, 0.0, 1.6), (1.5, 1.5, 1.5))
    place(parts["heal_cube"], (4.0, 0.0, 0.8), (1, 1, 1), rot_deg=(20, 0, 30))
    place(parts["shockwave"], (0.0, -6.0, 0.0), (3.0, 0.5, 3.0))
    place(parts["laser"], (6.0, -3.0, 1.5), (0.12, 0.12, 6.0), rot_deg=(0, 0, 60))
    place(parts["boss_shield"], (0.0, 3.0, 2.6), (2.6, 2.6, 1.0))
    c.render_view("Props", (0.0, 13.0, 5.0), (0.0, -1.5, 0.8), os.path.join(preview_dir, "props_overview.png"), lens=28.0)
    c.render_view("PropsClose", (-3.5, 4.5, 1.6), (-3.0, 0.0, 0.8), os.path.join(preview_dir, "props_close.png"), lens=35.0)


main()
