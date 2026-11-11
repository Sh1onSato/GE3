# アリーナの部品のモデルを生成する
#
# 実行: blender -b --factory-startup -P Tools/blender/arena.py -- Resources/models/arena <プレビュー出力先>
#
# 大きさの決まり（重要）:
#   今のアリーナは、配布の cube.obj（-1〜1）を ArenaLayout の scale（= 当たり判定の half-extent）で拡大している。
#   ここでは各部品を「実寸」で作り、最後にその scale で割って -1〜1 に戻して書き出す（common.finish の engine_scale）。
#   ゲーム側は今の scale のまま SetScale すれば実寸に戻るので、当たり判定は変わらず、発光ラインも引き伸ばされない。
#   そのため部品ごとに「どの scale で使う前提か」が決まっている（下の PARTS を参照）。
#
#   floor          … 床（scale {30,1,30}）。上面に4m間隔の発光グリッドと、中央の8角形のマーカー
#   floor_tile     … データ破損で消える床タイル（scale {2,1,2}、4m四方）。今の床を分割するとき用
#   wall_z / wall_x… 外壁（scale {30,8,1} / {1,8,30}）。両面に幅木の発光ラインとパネルの継ぎ目
#   cover          … 中央のカバー箱（scale {1.5,2,1.5}）
#   platform_low   … 角のプラットフォーム下段（scale {3,1.25,3}）
#   platform_high  … 角のプラットフォーム上段（scale {1.5,1.25,1.5}）
#   data_platform  … 空中のデータ足場（scale {2,0.2,2}、4m四方・厚さ0.4m。新規なので大きさは仮）
#
# 当たり判定は箱（AABB）なので、どの部品も見た目が箱いっぱいに収まるように作っている
# （発光ラインの出っ張り 1cm 程度と、角の小さな面取りだけが例外）。

import math
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

BODY, TRIM, GLOW = 0, 1, 2
ARENA_GLOW = (0.0, 0.75, 1.0)     # アリーナの発光（シアン）
DATA_GLOW = (0.4, 1.0, 0.8)       # データ足場・破損タイルの発光（アリーナと見分けるため緑寄り）
GLOW_PROTRUDE = 0.012             # 発光ラインの出っ張り
OCT = 0.3927


def make_materials(prefix, glow_color, glow_strength, body=(0.05, 0.06, 0.08), trim=(0.12, 0.14, 0.18)):
    return [
        c.make_material(prefix + "_Body", body, metallic=0.3, roughness=0.7),
        c.make_material(prefix + "_Trim", trim, metallic=0.4, roughness=0.6),
        c.make_material(prefix + "_Glow", glow_color, emission=glow_color, emission_strength=glow_strength),
    ]


# ---------------------------------------------------------------------------
# 床
# ---------------------------------------------------------------------------
def build_floor(mats):
    hx, hz_up = 30.0, 1.0
    b = c.MeshBuilder()
    b.box((-hx, -hx, -hz_up), (hx, hx, hz_up), BODY)
    top = hz_up
    w = 0.04
    for i in range(-7, 8):  # 4m 間隔のグリッド（外壁の際 ±30 は避ける）
        p = i * 4.0
        b.box((p - w, -28.0, top), (p + w, 28.0, top + GLOW_PROTRUDE), GLOW)
        b.box((-28.0, p - w, top), (28.0, p + w, top + GLOW_PROTRUDE), GLOW)
    # 中央の8角形マーカー（半径6m）
    outer = c.ngon_profile(0.0, 0.0, 6.0, 8, rot=OCT)
    for i in range(8):
        (x0, y0), (x1, y1) = outer[i], outer[(i + 1) % 8]
        length = math.hypot(x1 - x0, y1 - y0)
        ang = math.degrees(math.atan2(y1 - y0, x1 - x0))
        mid = ((x0 + x1) * 0.5, (y0 + y1) * 0.5)
        m = c.Matrix.Translation((mid[0], mid[1], 0.0)) @ c.Matrix.Rotation(math.radians(ang), 4, "Z")
        b.box((-length * 0.5, -0.08, top), (length * 0.5, 0.08, top + GLOW_PROTRUDE * 1.5), GLOW, matrix=m)
    return b.finish("floor", mats, engine_scale=(30.0, 1.0, 30.0))


def build_floor_tile(mats):
    h, hz_up = 2.0, 1.0
    b = c.MeshBuilder()
    b.prism_z(c.chamfer_rect_profile(-h, h, -h, h, 0.04), -hz_up, hz_up, BODY)
    inset, w = 0.12, 0.05
    for s in (1.0, -1.0):  # 上面の縁取り
        b.box((-h + inset, s * (h - inset) - w, hz_up), (h - inset, s * (h - inset) + w, hz_up + GLOW_PROTRUDE), GLOW)
        b.box((s * (h - inset) - w, -h + inset, hz_up), (s * (h - inset) + w, h - inset, hz_up + GLOW_PROTRUDE), GLOW)
    return b.finish("floor_tile", mats, engine_scale=(h, hz_up, h))


# ---------------------------------------------------------------------------
# 外壁（両面に同じ模様を付けるので、4方向どの壁にも使える）
# ---------------------------------------------------------------------------
def build_wall(mats, along_x):
    length, hz_up, thick = 30.0, 8.0, 1.0
    b = c.MeshBuilder()
    b.box((-length, -thick, -hz_up), (length, thick, hz_up), BODY)
    floor_local = -hz_up + 1.0  # 床の上面（ワールド y=0）は壁の中心（y=7）から -7
    for s in (1.0, -1.0):
        face = s * thick
        out = face + s * GLOW_PROTRUDE
        y0, y1 = sorted((face, out))
        b.box((-length, y0, floor_local + 0.1), (length, y1, floor_local + 0.22), GLOW)  # 幅木の発光
        b.box((-length, y0, floor_local + 6.0), (length, y1, floor_local + 6.06), GLOW)  # 目線より上の細いライン
        y0t, y1t = sorted((face, face + s * 0.03))
        b.box((-length, y0t, hz_up - 0.4), (length, y1t, hz_up), TRIM)                   # 上端の縁
        for i in range(-4, 5):  # パネルの継ぎ目（6m 間隔）
            x = i * 6.0 + 3.0
            if abs(x) >= length:
                continue
            b.box((x - 0.06, y0t, floor_local), (x + 0.06, y1t, hz_up - 0.4), TRIM)
    if along_x:
        name, scale = "wall_z", (30.0, 8.0, 1.0)  # z=±30 に置く、X 方向に長い壁
    else:
        # X 方向に長い形を 90 度回して、Z 方向（Blender の Y）に長い壁にする
        c.bmesh.ops.rotate(b.bm, cent=(0, 0, 0), matrix=c.Matrix.Rotation(math.radians(90), 3, "Z"),
                           verts=b.bm.verts[:])
        name, scale = "wall_x", (1.0, 8.0, 30.0)  # x=±30 に置く、Z 方向に長い壁
    return b.finish(name, mats, engine_scale=scale)


# ---------------------------------------------------------------------------
# カバー箱・プラットフォーム（共通の作り：面取りした柱＋上端の発光帯＋角の縦ライン）
# ---------------------------------------------------------------------------
def build_block(mats, name, hx, hz_up, hy, chamfer, mid_band=True):
    b = c.MeshBuilder()
    b.prism_z(c.chamfer_rect_profile(-hx, hx, -hy, hy, chamfer), -hz_up, hz_up, BODY)
    t = GLOW_PROTRUDE
    b.band_around(hx, hy, hz_up - 0.16, hz_up - 0.08, t, GLOW, inset=chamfer)    # 上端の発光帯（乗れる面の目印）
    b.band_around(hx, hy, -hz_up, -hz_up + 0.12, 0.02, TRIM, inset=chamfer)      # 足元の縁
    if mid_band:
        b.band_around(hx, hy, -0.05, 0.05, t, GLOW, inset=chamfer)
    # 面取りした角に縦のトリム
    for sx in (1.0, -1.0):
        for sy in (1.0, -1.0):
            cx, cy = sx * (hx - chamfer * 0.5), sy * (hy - chamfer * 0.5)
            m = c.Matrix.Translation((cx, cy, 0.0)) @ c.Matrix.Rotation(math.radians(45 * sx * sy), 4, "Z")
            b.box((-0.03, -chamfer * 0.75, -hz_up + 0.12), (0.03, chamfer * 0.75, hz_up - 0.2), TRIM, matrix=m)
    return b.finish(name, mats, engine_scale=(hx, hz_up, hy))


def build_data_platform(mats):
    h, hz_up = 2.0, 0.2
    b = c.MeshBuilder()
    b.prism_z(c.chamfer_rect_profile(-h, h, -h, h, 0.15), -hz_up, hz_up, BODY)
    t = GLOW_PROTRUDE
    b.band_around(h, h, -0.05, 0.05, t, GLOW, inset=0.15)  # 側面の発光帯
    inset, w = 0.2, 0.04
    for s in (1.0, -1.0):  # 上面の縁取り
        b.box((-h + inset, s * (h - inset) - w, hz_up), (h - inset, s * (h - inset) + w, hz_up + t), GLOW)
        b.box((s * (h - inset) - w, -h + inset, hz_up), (s * (h - inset) + w, h - inset, hz_up + t), GLOW)
    # 下面のグリッド（下にいるプレイヤーから「足場がある」と分かるように）
    for i in (-1, 0, 1):
        p = i * 1.0
        b.box((p - w, -h + inset, -hz_up - t), (p + w, h - inset, -hz_up), GLOW)
        b.box((-h + inset, p - w, -hz_up - t), (h - inset, p + w, -hz_up), GLOW)
    return b.finish("data_platform", mats, engine_scale=(h, hz_up, h))


def build_data_pylon(mats):
    """データ足場の中央に立てる柱（1m角・高さ2.5m）。地上から足場の上面は見えないので、
    この柱の側面にポータルを貼って足場へ上がる。scale {0.5, 1.25, 0.5} で使う"""
    h, hz_up = 0.5, 1.25
    b = c.MeshBuilder()
    b.prism_z(c.chamfer_rect_profile(-h, h, -h, h, 0.06), -hz_up, hz_up, BODY)
    t = GLOW_PROTRUDE
    b.band_around(h, h, hz_up - 0.12, hz_up - 0.06, t, GLOW, inset=0.06)    # 上端
    b.band_around(h, h, -hz_up + 0.06, -hz_up + 0.12, t, GLOW, inset=0.06)  # 下端
    # 4つの面の中央に縦の発光ライン（「ここに貼れる」目印）
    for ang in (0, 90, 180, 270):
        m = c.Matrix.Rotation(math.radians(ang), 4, "Z")
        b.box((-0.03, h, -hz_up + 0.25), (0.03, h + t, hz_up - 0.25), GLOW, matrix=m)
    return b.finish("data_pylon", mats, engine_scale=(h, hz_up, h))


# ---------------------------------------------------------------------------
# プレビュー：ArenaLayout の配置どおりに並べて、アリーナ全体を確認する
# ---------------------------------------------------------------------------
def place(obj, engine_pos, engine_scale):
    """エンジン座標・scale で配置した複製を作る（Blender は (x, z, y) の並び）"""
    inst = c.bpy.data.objects.new(obj.name + "_inst", obj.data)
    inst.location = (engine_pos[0], engine_pos[2], engine_pos[1])
    inst.scale = (engine_scale[0], engine_scale[2], engine_scale[1])
    c.bpy.context.scene.collection.objects.link(inst)
    return inst


def build_preview_layout(p):
    place(p["floor"], (0, -1, 0), (30, 1, 30))
    place(p["wall_z"], (0, 7, 30), (30, 8, 1))
    place(p["wall_z"], (0, 7, -30), (30, 8, 1))
    place(p["wall_x"], (30, 7, 0), (1, 8, 30))
    place(p["wall_x"], (-30, 7, 0), (1, 8, 30))
    for pos in ((12, 1, 0), (-12, 1, 0), (0, 1, 12), (0, 1, -12)):
        place(p["cover"], pos, (1.5, 2, 1.5))
    for sx in (14, -14):
        for sz in (14, -14):
            place(p["platform_low"], (sx, 1.25, sz), (3, 1.25, 3))
            place(p["platform_high"], (sx, 3.75, sz), (1.5, 1.25, 1.5))
    # データ足場の配置例（位置は仮）
    for pos in ((-8, 6.5, 4), (8, 6.5, 4), (0, 7.0, -4)):
        place(p["data_platform"], pos, (2, 0.2, 2))
        place(p["data_pylon"], (pos[0], pos[1] + 0.2 + 1.25, pos[2]), (0.5, 1.25, 0.5))
    for obj in p.values():
        obj.hide_render = True  # 原点に置いた元の部品は写さない


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "arena"))

    c.reset_scene()
    arena = make_materials("Arena", ARENA_GLOW, 2.5)
    floor_mats = make_materials("Floor", ARENA_GLOW, 1.6, body=(0.04, 0.045, 0.06))
    data = make_materials("Data", DATA_GLOW, 3.0, body=(0.05, 0.08, 0.08), trim=(0.1, 0.18, 0.16))

    parts = {
        "floor": build_floor(floor_mats),
        "floor_tile": build_floor_tile(data),
        "wall_z": build_wall(arena, along_x=True),
        "wall_x": build_wall(arena, along_x=False),
        "cover": build_block(arena, "cover", 1.5, 2.0, 1.5, 0.15),
        "platform_low": build_block(arena, "platform_low", 3.0, 1.25, 3.0, 0.2),
        "platform_high": build_block(arena, "platform_high", 1.5, 1.25, 1.5, 0.15, mid_band=False),
        "data_platform": build_data_platform(data),
        "data_pylon": build_data_pylon(data),
    }
    c.save_blend(out_dir, "arena")
    for name, obj in parts.items():
        c.export_glb(obj, out_dir, name)

    c.setup_preview_scene()
    build_preview_layout(parts)
    c.render_view("ArenaOverview", (-26.0, -26.0, 20.0), (0.0, 0.0, 0.0),
                  os.path.join(preview_dir, "arena_overview.png"), lens=20.0)
    # プレイヤーのスポーン付近（z=-8）の目線からボス側を見る
    c.render_view("ArenaPlayerView", (0.0, -8.0, 1.5), (0.0, 20.0, 2.0),
                  os.path.join(preview_dir, "arena_player.png"), lens=18.0)
    # 中央のカバー箱（エンジン座標 (0,1,-12) → Blender (0,-12,1)）の近く
    c.render_view("CoverClose", (5.0, -6.0, 3.0), (0.0, -12.0, 1.0),
                  os.path.join(preview_dir, "arena_cover.png"), lens=35.0)


main()
