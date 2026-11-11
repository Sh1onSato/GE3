# ボス（立方体型）のモデルを生成する
#
# 実行: blender -b --factory-startup -P Tools/blender/boss_cube.py -- Resources/models/boss_cube <プレビュー出力先>
#
# 形: 4m 角の立方体が、正面から見て X 字に4つのくさび（上・下・左・右）に割れていて、
#     中心の空洞にコアが埋まっている。開くと、くさびが上下左右へ「かぱっ」と離れてコアが出てくる。
#     閉じた状態ではコアは完全に隠れ、継ぎ目（隙間 GAP）からだけ内側の光が漏れる。
#
# 大きさ・原点（今の当たり判定 幅4×高さ5×奥行4 に収める）:
#   原点 = 当たり判定の箱の中心（底面 -2.5、上面 +2.5）。ゲーム側ではスケール {1,1,1} で、今と同じ位置に置く。
#   立方体の中心は原点から上に CUBE_CENTER_Z（浮いて見えるように床から 1m あける）。
#   正面 = Blender の +Y = エンジンの +Z（今の SetRotate の向きと同じ）。
#
# パーツ（すべて同じ原点で書き出しているので、ゲーム側では全パーツに同じ位置・回転を与えれば組み上がる）:
#   boss_cube_wedge_top / _bottom / _left / _right … くさび。開くときは、それぞれ
#       上(+Y) / 下(-Y) / 左(-X) / 右(+X)（ボスのローカル座標・エンジンの軸）へ OPEN_DISTANCE だけ動かす
#   boss_cube_core … コア（中心の8面体）
#
# マテリアル:
#   Boss_Body  … 外側の面（暗い色）
#   Boss_Inner … 割れ目・空洞に面した内側の面（弱く発光。開いたときに中が光って見える）
#   Boss_Glow  … 外側の発光ライン（P3 の赤化や攻撃の予兆はコードで発光色を変える）

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

HALF = 2.0            # 立方体の半分の大きさ（4m 角）
CUBE_CENTER_Z = 0.5   # 原点から立方体の中心までの高さ（立方体の下面は床から 1m 浮く）
CAVITY = 0.7          # コアを入れる空洞の半分の大きさ
GAP = 0.025           # くさび同士の隙間（継ぎ目から光が漏れる）
OPEN_DISTANCE = 1.3   # 開いたときに、くさびを外へ動かす距離（ゲーム側の定数と合わせる）
                      # 下のくさびは床から 1m 浮いているので、下には 1.0 未満しか下げられない点に注意
                      # （ゲーム側で下だけ距離を短くするか、開くときにボスごと少し浮かせる）
SLICE_STEP = 0.15     # 実体化演出の横ズレ用の輪切り間隔
# 当たり判定（GameScene::GetAABB）は Object3d の scale を half-extent として計算しているため、
# ゲーム側はボスの scale を今の ArenaLayout::kBossScale {2,2.5,2} のまま使う。
# そこで実寸で作った形をこの値で割って書き出す（common.finish の engine_scale）。
ENGINE_SCALE = (2.0, 2.5, 2.0)

BODY, INNER, GLOW = 0, 1, 2
GLOW_COLOR = (0.75, 0.15, 1.0)
INNER_COLOR = (0.12, 0.03, 0.2)   # 内側は暗めの紫でほんのり光らせ、コアを主役にする
CORE_COLOR = (1.0, 0.3, 0.75)

# くさびの名前と、正面から見た回転角（Blender の Y 軸回り。0 度 = 上のくさび）
WEDGES = {
    "boss_cube_wedge_top": 0.0,
    "boss_cube_wedge_right": 90.0,
    "boss_cube_wedge_bottom": 180.0,
    "boss_cube_wedge_left": 270.0,
}
# 開くときの移動方向（Blender 座標。プレビュー用。エンジンでは上下が Y、左右が X）
OPEN_DIRECTION = {
    "boss_cube_wedge_top": (0.0, 0.0, 1.0),
    "boss_cube_wedge_right": (1.0, 0.0, 0.0),
    "boss_cube_wedge_bottom": (0.0, 0.0, -1.0),
    "boss_cube_wedge_left": (-1.0, 0.0, 0.0),
}


def make_materials():
    return [
        c.make_material("Boss_Body", (0.06, 0.05, 0.08), metallic=0.5, roughness=0.45),
        c.make_material("Boss_Inner", INNER_COLOR, emission=INNER_COLOR, emission_strength=2.0),
        c.make_material("Boss_Glow", GLOW_COLOR, emission=GLOW_COLOR, emission_strength=4.0),
    ]


def build_wedge(mats, name, angle_deg):
    """上のくさびを作ってから、正面から見た回転で右・下・左のくさびにする。
    正面側・背面側は中心まで届く三角形、真ん中の奥行き（±CAVITY）だけ先端を切った台形にして、
    閉じたときに中心の空洞（コアの入れ物）が外から見えないようにしている。"""
    h, r, g = HALF, CAVITY, GAP
    tri = [(-h, h + g), (h, h + g), (0.0, g)]
    trap = [(-h, h + g), (h, h + g), (r, r + g), (-r, r + g)]
    m = c.Matrix.Translation((0.0, 0.0, CUBE_CENTER_Z)) @ c.Matrix.Rotation(c.math.radians(angle_deg), 4, "Y")

    b = c.MeshBuilder()
    b.prism_y(tri, r, h, BODY, matrix=m)     # 正面側
    b.prism_y(tri, -h, -r, BODY, matrix=m)   # 背面側
    b.prism_y(trap, -r, r, BODY, matrix=m)   # 真ん中（空洞の天井になる）

    # 外側の面の発光ライン（十字）と、正面・背面の三角形の縁に沿ったライン
    t = 0.012
    top = h + g
    b.box((-0.05, -h + 0.3, top), (0.05, h - 0.3, top + t), GLOW, matrix=m)
    b.box((-h + 0.3, -0.05, top), (h - 0.3, 0.05, top + t), GLOW, matrix=m)
    b.box((-1.3, h, top - 0.35), (1.3, h + t, top - 0.25), GLOW, matrix=m)
    b.box((-1.3, -h - t, top - 0.35), (1.3, -h, top - 0.25), GLOW, matrix=m)

    mark_inner_faces(b.bm, m)
    return b.finish(name, mats, slice_step=SLICE_STEP, engine_scale=ENGINE_SCALE)


def mark_inner_faces(bm, m):
    """立方体の外側に出ていない面（割れ目・空洞に面した面）を Boss_Inner にする"""
    c.bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    inv = m.inverted()
    limit = HALF - 0.01
    for f in bm.faces:
        if f.material_index != BODY:
            continue
        p = inv @ f.calc_center_median()  # くさびを回す前の座標（上のくさびの向き）で判定する
        on_outer = (p.z > HALF + GAP - 0.01) or abs(p.y) > limit
        if not on_outer:
            f.material_index = INNER


def build_core():
    b = c.MeshBuilder()
    core_mat = c.make_material("Boss_Core", CORE_COLOR, emission=CORE_COLOR, emission_strength=8.0)
    c.octahedron(b, (0.0, 0.0, CUBE_CENTER_Z), CAVITY * 0.85, CAVITY * 0.85, 0)
    return b.finish("boss_cube_core", [core_mat], slice_step=SLICE_STEP, engine_scale=ENGINE_SCALE)


def set_open(parts, amount):
    for name, d in OPEN_DIRECTION.items():
        parts[name].location = tuple(v * amount for v in d)


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "boss_cube"))

    c.reset_scene()
    mats = make_materials()
    parts = {name: build_wedge(mats, name, angle) for name, angle in WEDGES.items()}
    parts["boss_cube_core"] = build_core()

    c.save_blend(out_dir, "boss_cube")
    for name, obj in parts.items():
        c.export_glb(obj, out_dir, name)

    c.setup_preview_scene()
    for obj in parts.values():  # ゲームと同じ scale に戻して写す（Blender は (x, z, y) の並び）
        obj.scale = (ENGINE_SCALE[0], ENGINE_SCALE[2], ENGINE_SCALE[1])
    c.render_view("Closed", (7.0, 9.0, 2.5), (0.0, 0.0, 0.3), os.path.join(preview_dir, "bosscube_closed.png"))
    set_open(parts, OPEN_DISTANCE)
    c.render_view("Open", (7.0, 9.0, 2.5), (0.0, 0.0, 0.3), os.path.join(preview_dir, "bosscube_open.png"))
    c.render_view("OpenFront", (0.0, 11.0, 0.8), (0.0, 0.0, 0.5), os.path.join(preview_dir, "bosscube_open_front.png"))
    # データ足場（高さ 6〜8m）から見下ろす想定
    c.render_view("OpenAbove", (4.0, 7.0, 8.0), (0.0, 0.0, 0.5), os.path.join(preview_dir, "bosscube_open_above.png"))
    set_open(parts, 0.0)


main()
