# Blender モデル生成スクリプトの共通処理
#
# 座標の決まり（S1Engine の GltfLoader の変換を前提にしている）:
#   Blender の +X = エンジンの右 (+X)
#   Blender の +Y = エンジンの前 (+Z)
#   Blender の +Z = エンジンの上 (+Y)
#   単位はメートル（エンジンの 1.0 = Blender の 1m）
#
# 使い方: 各モデルのスクリプト（rifle.py など）から import して使う。
#   blender -b --factory-startup -P Tools/blender/rifle.py -- <出力フォルダ> <プレビュー画像フォルダ>

import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector


# ---------------------------------------------------------------------------
# 引数・シーン
# ---------------------------------------------------------------------------
def parse_args(default_out_dir):
    """'--' 以降の引数から (出力フォルダ, プレビューフォルダ) を返す"""
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out_dir = os.path.abspath(argv[0]) if len(argv) >= 1 else os.path.abspath(default_out_dir)
    preview_dir = os.path.abspath(argv[1]) if len(argv) >= 2 else out_dir
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(preview_dir, exist_ok=True)
    return out_dir, preview_dir


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


# ---------------------------------------------------------------------------
# マテリアル
# ---------------------------------------------------------------------------
def make_material(name, base, metallic=0.0, roughness=0.5, emission=None, emission_strength=0.0):
    """Principled BSDF のマテリアルを作る。emission を渡すと発光（glTF の emissive）になる"""
    mat = bpy.data.materials.new(name)
    try:
        mat.use_nodes = True  # Blender 5 では常にノード使用のため不要だが、古い版向けに残す
    except Exception:
        pass
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = (*base, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if emission is not None:
        bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
        # 1.0 を超えると KHR_materials_emissive_strength として書き出される（ブルームに乗せる用）
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    mat.diffuse_color = (*base, 1.0)  # ビューポート表示用
    return mat


# ---------------------------------------------------------------------------
# 形状の組み立て
# ---------------------------------------------------------------------------
def rect_profile(x0, x1, z0, z1):
    return [(x0, z0), (x1, z0), (x1, z1), (x0, z1)]


def chamfer_rect_profile(x0, x1, z0, z1, c):
    """角を c だけ斜めに落とした長方形（8角形）"""
    return [(x0 + c, z0), (x1 - c, z0), (x1, z0 + c), (x1, z1 - c),
            (x1 - c, z1), (x0 + c, z1), (x0, z1 - c), (x0, z0 + c)]


def ngon_profile(cx, cz, radius, sides, rot=0.0):
    return [(cx + radius * math.cos(rot + 2 * math.pi * i / sides),
             cz + radius * math.sin(rot + 2 * math.pi * i / sides)) for i in range(sides)]


class MeshBuilder:
    """複数のパーツを1つのメッシュに積み上げる。パーツごとにマテリアル番号を持つ"""

    def __init__(self):
        self.bm = bmesh.new()

    def prism_y(self, profile, y0, y1, mat_index, matrix=None):
        """XZ 平面の輪郭 profile を Y 方向 y0→y1 に押し出した柱を作る"""
        bm = self.bm
        back = [bm.verts.new((x, y0, z)) for x, z in profile]
        front = [bm.verts.new((x, y1, z)) for x, z in profile]
        faces = [bm.faces.new(front), bm.faces.new(list(reversed(back)))]
        n = len(profile)
        for i in range(n):
            j = (i + 1) % n
            faces.append(bm.faces.new((back[i], back[j], front[j], front[i])))
        for f in faces:
            f.material_index = mat_index
        if matrix is not None:
            bmesh.ops.transform(bm, matrix=matrix, verts=back + front)
        return faces

    def prism_z(self, profile_xy, z0, z1, mat_index):
        """XY 平面の輪郭を Z（エンジンの上方向）に z0→z1 で押し出す（柱・土台など）"""
        bm = self.bm
        low = [bm.verts.new((x, y, z0)) for x, y in profile_xy]
        high = [bm.verts.new((x, y, z1)) for x, y in profile_xy]
        faces = [bm.faces.new(high), bm.faces.new(list(reversed(low)))]
        n = len(profile_xy)
        for i in range(n):
            j = (i + 1) % n
            faces.append(bm.faces.new((low[i], low[j], high[j], high[i])))
        for f in faces:
            f.material_index = mat_index
        return faces

    def band_around(self, hx, hy, z0, z1, thickness, mat_index, inset=0.0):
        """中心原点・半幅 (hx, hy) の箱の4つの側面に、高さ z0〜z1 の帯を巻く（発光ラインなど）。
        帯は面の外側に thickness だけ出す。角を面取りした箱では inset に面取り幅を渡すと、
        帯の長さを平らな面の範囲に収める"""
        t = thickness
        ex, ey = hx - inset, hy - inset
        self.box((hx, -ey, z0), (hx + t, ey, z1), mat_index)
        self.box((-hx - t, -ey, z0), (-hx, ey, z1), mat_index)
        self.box((-ex, hy, z0), (ex, hy + t, z1), mat_index)
        self.box((-ex, -hy - t, z0), (ex, -hy, z1), mat_index)

    def box(self, mn, mx, mat_index, matrix=None):
        """軸に沿った箱。mn/mx は (x, y, z)"""
        return self.prism_y(rect_profile(mn[0], mx[0], mn[2], mx[2]), mn[1], mx[1], mat_index, matrix)

    def box_mirror_x(self, mn, mx, mat_index):
        """X の正側に置いた箱と、その左右反転を作る（左右対称の発光ラインなど）"""
        self.box(mn, mx, mat_index)
        self.box((-mx[0], mn[1], mn[2]), (-mn[0], mx[1], mx[2]), mat_index)

    def finish(self, name, materials, slice_step=None, engine_scale=None):
        """メッシュを確定してオブジェクトを作る。法線を外向きにそろえ、UV を付ける。
        slice_step を渡すと、高さ方向にその間隔で輪切りにして頂点を増やす
        （ボスの実体化演出の、頂点シェーダーでの横ズレを効かせるため）。
        engine_scale=(x, y上, z) を渡すと、実寸で作った形をその値で割って書き出す。
        ゲーム側でその値を SetScale すると実寸に戻るので、当たり判定用の今の scale を変えずに済み、
        発光ラインも引き伸ばされない（配布の cube.obj と同じ -1〜1 の決まりに合わせるため）。"""
        bm = self.bm
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        if slice_step:
            slice_horizontally(bm, slice_step)
        add_box_projected_uv(bm)
        if engine_scale is not None:
            sx, sy_up, sz = engine_scale
            bmesh.ops.scale(bm, vec=(1.0 / sx, 1.0 / sz, 1.0 / sy_up), verts=bm.verts[:])
        mesh = bpy.data.meshes.new(name)
        bm.to_mesh(mesh)
        bm.free()
        for m in materials:
            mesh.materials.append(m)
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def rotation_about(axis, degrees, pivot):
    """pivot を中心に axis('X'/'Y'/'Z') 回りに回す行列"""
    p = Vector(pivot)
    return Matrix.Translation(p) @ Matrix.Rotation(math.radians(degrees), 4, axis) @ Matrix.Translation(-p)


def octahedron(builder, center, radius, height, mat_index):
    """上下にとがった8面体（コアなど）。radius は横の半径、height は中心から頂点までの高さ"""
    bm = builder.bm
    cx, cy, cz = center
    ring = [bm.verts.new((cx + radius * math.cos(a), cy + radius * math.sin(a), cz))
            for a in (0.0, math.pi / 2, math.pi, math.pi * 1.5)]
    top = bm.verts.new((cx, cy, cz + height))
    bottom = bm.verts.new((cx, cy, cz - height))
    for i in range(4):
        j = (i + 1) % 4
        bm.faces.new((ring[i], ring[j], top)).material_index = mat_index
        bm.faces.new((ring[j], ring[i], bottom)).material_index = mat_index


def slice_horizontally(bm, step):
    """メッシュ全体を Z（エンジンの上方向）に step 間隔で輪切りにする"""
    zs = [v.co.z for v in bm.verts]
    z = min(zs) + step
    top = max(zs)
    while z < top - 1e-4:
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geom, plane_co=(0.0, 0.0, z), plane_no=(0.0, 0.0, 1.0))
        z += step


def add_box_projected_uv(bm, scale=1.0):
    """面の向きに合わせて箱投影の UV を付ける（テクスチャ無しでも UV 必須のローダー向け）"""
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for loop in f.loops:
            co = loop.vert.co
            if ax == 0:
                loop[uv].uv = (co.y * scale, co.z * scale)
            elif ax == 1:
                loop[uv].uv = (co.x * scale, co.z * scale)
            else:
                loop[uv].uv = (co.x * scale, co.y * scale)


# ---------------------------------------------------------------------------
# 書き出し・プレビュー
# ---------------------------------------------------------------------------
def save_blend(out_dir, base_name):
    path = os.path.join(out_dir, base_name + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=path)
    print(f"[model] saved: {path}")


def export_glb(obj, out_dir, base_name):
    """obj だけを .glb に書き出す"""
    path = os.path.join(out_dir, base_name + ".glb")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_apply=True)
    print(f"[model] exported: {path}")
    return path


def save_and_export(obj, out_dir, base_name):
    """.blend と .glb を書き出す（.blend はプレビュー用のカメラ等を入れる前に保存する）"""
    blend_path = os.path.join(out_dir, base_name + ".blend")
    glb_path = os.path.join(out_dir, base_name + ".glb")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)

    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=glb_path, export_format="GLB", use_selection=True, export_apply=True)
    print(f"[model] saved: {blend_path}")
    print(f"[model] exported: {glb_path}")
    return glb_path


def _set_render_engine(scene):
    for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE", "BLENDER_WORKBENCH"):
        try:
            scene.render.engine = engine
            return engine
        except TypeError:
            continue
    return scene.render.engine


def setup_preview_scene(background=(0.02, 0.025, 0.035)):
    scene = bpy.context.scene
    engine = _set_render_engine(scene)
    scene.render.resolution_x = 960
    scene.render.resolution_y = 540
    scene.render.film_transparent = False

    world = bpy.data.worlds.new("PreviewWorld")
    scene.world = world
    try:
        world.use_nodes = True
    except Exception:
        pass
    bg = next((n for n in world.node_tree.nodes if n.type == "BACKGROUND"), None)
    if bg is not None:
        bg.inputs["Color"].default_value = (*background, 1.0)
        bg.inputs["Strength"].default_value = 1.0

    def add_light(name, kind, energy, loc, rot_deg):
        data = bpy.data.lights.new(name, kind)
        data.energy = energy
        ob = bpy.data.objects.new(name, data)
        ob.location = loc
        ob.rotation_euler = tuple(math.radians(a) for a in rot_deg)
        scene.collection.objects.link(ob)

    add_light("Key", "SUN", 3.0, (0, 0, 5), (50, 10, 30))
    add_light("Fill", "SUN", 0.8, (0, 0, 5), (60, 0, 200))
    print(f"[preview] render engine: {engine}")
    return scene


def render_view(name, location, target, out_path, lens=35.0):
    scene = bpy.context.scene
    cam_data = bpy.data.cameras.new(name)
    cam_data.lens = lens
    cam_data.clip_start = 0.01
    cam = bpy.data.objects.new(name, cam_data)
    cam.location = location
    direction = Vector(target) - Vector(location)
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(cam)
    scene.camera = cam
    scene.render.filepath = out_path
    bpy.ops.render.render(write_still=True)
    print(f"[preview] rendered: {out_path}")
