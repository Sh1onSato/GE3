# 視点武器（ライフル）のモデルを生成する
#
# 実行: blender -b --factory-startup -P Tools/blender/rifle.py -- Resources/models/rifle <プレビュー出力先>
#
# 原点 = 本体の中心。ゲーム側では今の weaponBodyLocalOffset {0.22, -0.20, 0.45} に置く想定。
# 銃口の先端 = 原点から 前 0.33 / 上 0.02（エンジン座標で {0, 0.02, 0.33}）
#   → カメラから見ると {0.22, -0.18, 0.78} で、今の銃口位置（weaponBarrelLocalOffset.z + 0.16）と一致する。
# 座標の決まりは common.py の先頭を参照（Blender +Y = 前）。

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

MUZZLE_Y = 0.33   # 銃口の先端（前方向）
MUZZLE_Z = 0.02   # 銃口の高さ

BODY, TRIM, GLOW = 0, 1, 2
GLOW_COLOR = (0.0, 0.85, 1.0)  # シアン（世界観の発光色）
GLOW_STRENGTH = 4.0            # ブルームのしきい値 1.0 を超えて光らせる


def build_rifle():
    mats = [
        c.make_material("Rifle_Body", (0.06, 0.065, 0.08), metallic=0.6, roughness=0.45),
        c.make_material("Rifle_Trim", (0.16, 0.17, 0.20), metallic=0.4, roughness=0.6),
        c.make_material("Rifle_Glow", GLOW_COLOR, emission=GLOW_COLOR, emission_strength=GLOW_STRENGTH),
    ]
    b = c.MeshBuilder()

    # --- 本体まわり ---
    b.prism_y(c.chamfer_rect_profile(-0.035, 0.035, -0.045, 0.045, 0.012), -0.20, 0.16, BODY)  # 機関部
    b.prism_y(c.chamfer_rect_profile(-0.028, 0.028, -0.030, 0.040, 0.010), 0.16, 0.24, TRIM)   # ハンドガード
    b.prism_y(c.chamfer_rect_profile(-0.025, 0.025, -0.050, 0.035, 0.010), -0.32, -0.20, TRIM)  # ストック
    b.box((-0.015, -0.14, 0.045), (0.015, 0.14, 0.058), TRIM)                                   # 上部レール

    # --- 銃身・銃口 ---
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.016, 6, rot=0.5236), 0.24, 0.30, BODY)            # 銃身
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.024, 6, rot=0.5236), 0.30, MUZZLE_Y, TRIM)        # マズル
    b.prism_y(c.ngon_profile(0.0, MUZZLE_Z, 0.027, 6, rot=0.5236), 0.312, 0.318, GLOW)          # マズルの発光リング

    # --- サイト（照準器）。発光はプレイヤーから見える後ろ側に付ける ---
    b.box((-0.018, 0.02, 0.058), (0.018, 0.07, 0.085), TRIM)
    b.box((-0.010, 0.017, 0.064), (0.010, 0.020, 0.080), GLOW)

    # --- マガジン（少し前に傾ける）と発光する残弾ライン ---
    mag_tilt = c.rotation_about("X", 10.0, (0.0, 0.035, -0.045))
    b.box((-0.022, 0.0, -0.13), (0.022, 0.07, -0.045), BODY, matrix=mag_tilt)
    b.box((0.022, 0.03, -0.115), (0.025, 0.04, -0.06), GLOW, matrix=mag_tilt)
    b.box((-0.025, 0.03, -0.115), (-0.022, 0.04, -0.06), GLOW, matrix=mag_tilt)

    # --- グリップ（後ろに傾ける） ---
    grip_tilt = c.rotation_about("X", -15.0, (0.0, -0.107, -0.045))
    b.box((-0.018, -0.13, -0.12), (0.018, -0.085, -0.045), TRIM, matrix=grip_tilt)

    # --- 側面の発光ライン（左右対称） ---
    b.box_mirror_x((0.035, -0.17, 0.008), (0.038, 0.14, 0.014), GLOW)    # 機関部の上ライン
    b.box_mirror_x((0.035, -0.05, -0.020), (0.038, 0.12, -0.016), GLOW)  # 機関部の下ライン
    b.box_mirror_x((0.028, 0.17, 0.000), (0.031, 0.23, 0.006), GLOW)     # ハンドガードのスリット

    return b.finish("Rifle", mats)


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, preview_dir = c.parse_args(os.path.join(project_root, "Resources", "models", "rifle"))

    c.reset_scene()
    rifle = build_rifle()
    c.save_and_export(rifle, out_dir, "rifle")

    # プレビュー：全体（斜め前）と、ゲーム中のカメラ位置から見た図
    c.setup_preview_scene()
    c.render_view("Overview", (0.55, 0.45, 0.30), (0.0, 0.0, 0.0),
                  os.path.join(preview_dir, "rifle_overview.png"), lens=50.0)
    # ゲーム中、カメラは銃の原点から 左 0.22 / 後ろ 0.45 / 上 0.20 にあり、前を向いている
    c.render_view("FirstPerson", (-0.22, -0.45, 0.20), (-0.22, 1.0, 0.20),
                  os.path.join(preview_dir, "rifle_fps.png"), lens=24.0)


main()
