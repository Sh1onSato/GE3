# 視点武器（ショットガン）のモデルを生成する
#
# 実行: blender -b --factory-startup -P Tools/blender/shotgun.py -- Resources/models/shotgun <プレビュー出力先>
#
# rifle.py と同じ決まりで作っている（差し替えて見比べられるように）:
#   原点 = 本体の中心。ゲーム側では今の weaponBodyLocalOffset {0.22, -0.20, 0.45} に置く想定。
#   銃口の先端 = 原点から 前 0.33 / 上 0.02（エンジン座標で {0, 0.02, 0.33}）。
# 見た目の方針: 太い銃身・銃身下のチューブ弾倉・ポンプ式の先台・側面のシェル残数表示で「単発で重い一撃」に見せる。

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

MUZZLE_Y = 0.33
MUZZLE_Z = 0.02

BODY, TRIM, GLOW = 0, 1, 2
GLOW_COLOR = (0.0, 0.85, 1.0)
GLOW_STRENGTH = 4.0
OCT = 0.3927  # 8角形を平らな面が上に来る向きにする回転（22.5度）


def build_shotgun():
    mats = [
        c.make_material("Shotgun_Body", (0.06, 0.065, 0.08), metallic=0.6, roughness=0.45),
        c.make_material("Shotgun_Trim", (0.16, 0.17, 0.20), metallic=0.4, roughness=0.6),
        c.make_material("Shotgun_Glow", GLOW_COLOR, emission=GLOW_COLOR, emission_strength=GLOW_STRENGTH),
    ]
    b = c.MeshBuilder()

    # --- 機関部（ライフルより太く短い）・ストック・グリップ ---
    b.prism_y(c.chamfer_rect_profile(-0.04, 0.04, -0.05, 0.05, 0.014), -0.16, 0.10, BODY)
    b.prism_y(c.chamfer_rect_profile(-0.028, 0.028, -0.06, 0.035, 0.012), -0.30, -0.16, TRIM)
    grip_tilt = c.rotation_about("X", -18.0, (0.0, -0.128, -0.05))
    b.box((-0.02, -0.15, -0.125), (0.02, -0.105, -0.05), TRIM, matrix=grip_tilt)

    # --- 太い銃身と銃口 ---
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.030, 8, rot=OCT), 0.10, 0.29, BODY)
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.038, 8, rot=OCT), 0.29, MUZZLE_Y, TRIM)
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.041, 8, rot=OCT), 0.300, 0.306, GLOW)  # 銃口の発光リング
    b.box((-0.008, 0.10, 0.048), (0.008, 0.29, 0.056), TRIM)                          # 銃身上のベンチリブ
    b.box((-0.006, 0.282, 0.056), (0.006, 0.29, 0.066), GLOW)                         # 先端の照星

    # --- 銃身下のチューブ弾倉（先台は build_pump() で別パーツにしている） ---
    b.prism_y(c.ngon_profile(0.0, -0.03, 0.018, 8, rot=OCT), 0.06, 0.28, TRIM)

    # --- 照門（後ろ側のノッチを光らせる） ---
    b.box((-0.016, -0.06, 0.05), (0.016, -0.03, 0.07), TRIM)
    b.box((-0.005, -0.063, 0.058), (0.005, -0.06, 0.068), GLOW)

    # --- 側面：シェルの残数表示（4つ）と機関部のライン ---
    for i in range(4):
        y0 = -0.13 + i * 0.035
        b.box_mirror_x((0.04, y0, -0.03), (0.043, y0 + 0.022, 0.0), GLOW)
    b.box_mirror_x((0.04, -0.15, 0.022), (0.043, 0.08, 0.028), GLOW)
    b.box_mirror_x((0.028, -0.28, -0.01), (0.031, -0.18, -0.004), GLOW)  # ストックのライン
    b.box_mirror_x((0.029, 0.12, 0.03), (0.032, 0.27, 0.036), GLOW)       # 銃身のライン

    return b.finish("Shotgun", mats), mats


def build_pump(mats):
    """ポンプ式の先台。本体と同じ原点で書き出すので、ゲーム側では本体と同じ位置・回転に置き、
    発射後に銃の前後方向（エンジンの -Z）へ PUMP_STROKE だけ引いて戻すとポンプ動作になる。
    通常位置 y 0.16〜0.27、引いた位置 y 0.11〜0.22 で、どちらも機関部の前端（y 0.10）にめり込まず、
    チューブ弾倉（y 0.06〜0.28）の上に乗ったままになる。"""
    b = c.MeshBuilder()
    b.prism_y(c.chamfer_rect_profile(-0.033, 0.033, -0.06, -0.004, 0.012), 0.16, 0.27, TRIM)
    for y0 in (0.175, 0.20, 0.225, 0.25):  # 先台の溝（左右とも発光）
        b.box_mirror_x((0.033, y0, -0.045), (0.036, y0 + 0.01, -0.018), GLOW)
    return b.finish("ShotgunPump", mats)


PUMP_STROKE = 0.05  # 先台を引く距離（m）。ゲーム側の定数と合わせる


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "shotgun"))

    c.reset_scene()
    gun, mats = build_shotgun()
    pump = build_pump(mats)
    c.save_blend(out_dir, "shotgun")
    c.export_glb(gun, out_dir, "shotgun")
    c.export_glb(pump, out_dir, "shotgun_pump")

    c.setup_preview_scene()
    # 先台を引いた状態の確認用
    pump.location.y = -PUMP_STROKE
    c.render_view("LeftSidePumped", (-0.75, 0.05, 0.12), (0.0, 0.0, 0.0),
                  os.path.join(preview_dir, "shotgun_left_pumped.png"), lens=50.0)
    pump.location.y = 0.0
    c.render_view("Overview", (0.55, 0.45, 0.30), (0.0, 0.0, 0.0),
                  os.path.join(preview_dir, "shotgun_overview.png"), lens=50.0)
    c.render_view("LeftSide", (-0.75, 0.05, 0.12), (0.0, 0.0, 0.0),
                  os.path.join(preview_dir, "shotgun_left.png"), lens=50.0)
    c.render_view("FirstPerson", (-0.22, -0.45, 0.20), (-0.22, 1.0, 0.20),
                  os.path.join(preview_dir, "shotgun_fps.png"), lens=24.0)


main()
