# HUD 用の武器アイコン（PNG）を生成する
#
# 実行: blender -b --factory-startup -P Tools/blender/weapon_icons.py -- Resources/ui
#   （先に shotgun.py / rifle.py を実行して .glb を作っておくこと）
#
# 出力（背景透明、512x192）:
#   weapon_shotgun.png / weapon_shotgun_gray.png … ショットガン（先台込み）。_gray は今持っていない方の表示用
#   weapon_rifle.png   / weapon_rifle_gray.png   … ポータルガン（ライフルのモデル）
# 真横（エンジンの +X 側）から平行投影で写し、銃口が右を向くようにしている。

import os
import sys

import bpy

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import common as c  # noqa: E402

WIDTH, HEIGHT = 512, 192
ORTHO_SCALE = 0.72   # 写す範囲の横幅（m）。銃の全長（約0.65m）が収まる
CENTER_Y = 0.005     # 銃の前後方向の中心（Blender の Y）


def import_glb(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before]


def setup_render():
    scene = bpy.context.scene
    c.setup_preview_scene()
    scene.render.resolution_x = WIDTH
    scene.render.resolution_y = HEIGHT
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    try:
        scene.view_settings.view_transform = "Standard"  # 発光色をそのままの色で出す
    except TypeError:
        pass

    cam_data = bpy.data.cameras.new("IconCam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = ORTHO_SCALE
    cam = bpy.data.objects.new("IconCam", cam_data)
    # +X 側から -X を向く（画面の右 = Blender の +Y = 銃口側）
    cam.location = (1.0, CENTER_Y, 0.0)
    direction = c.Vector((-1.0, 0.0, 0.0))
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam
    return scene


def render_to(path):
    scene = bpy.context.scene
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print(f"[icon] rendered: {path}")


def save_grayscale(src_path, dst_path):
    """輝度だけにしたグレースケール版を作る（アルファはそのまま）"""
    img = bpy.data.images.load(src_path)
    px = list(img.pixels)
    for i in range(0, len(px), 4):
        lum = 0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2]
        px[i] = px[i + 1] = px[i + 2] = lum
    gray = bpy.data.images.new("gray", width=img.size[0], height=img.size[1], alpha=True)
    gray.pixels = px
    gray.filepath_raw = dst_path
    gray.file_format = "PNG"
    gray.save()
    print(f"[icon] grayscale: {dst_path}")


def main():
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out_dir, _ = c.parse_args(os.path.join(project_root, "Resources", "ui"))
    models = os.path.join(project_root, "Resources", "models")

    c.reset_scene()
    setup_render()

    sets = {
        "weapon_shotgun": [os.path.join(models, "shotgun", "shotgun.glb"), os.path.join(models, "shotgun", "shotgun_pump.glb")],
        "weapon_rifle": [os.path.join(models, "rifle", "rifle.glb")],
    }
    for name, files in sets.items():
        objs = []
        for f in files:
            objs += import_glb(f)
        path = os.path.join(out_dir, name + ".png")
        render_to(path)
        save_grayscale(path, os.path.join(out_dir, name + "_gray.png"))
        for o in objs:  # 次の銃を写す前に消す
            bpy.data.objects.remove(o, do_unlink=True)


main()
