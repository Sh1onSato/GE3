#pragma once
#include "Collision.h"
#include "Structs.h"
#include <string>
#include <vector>

/// <summary>
/// 高さの格子で表した地形（ハイトマップ）。高さ・地面の向き・レイの当たりを調べる（描画はしない。見た目は同じ格子から作った .glb を Object3d で描く）。
///
/// データは Blender のスクリプトが書き出す2つのファイル:
///   ・設定の JSON（点の数 samplesX / samplesZ・間隔 cellSize・左下の位置 originXZ・高さのファイル名 heightFile・答え合わせの点 checkpoints）
///   ・高さのファイル（float32 リトルエンディアン、index = z * samplesX + x。JSON と同じフォルダに置く）
/// 座標はエンジンの座標（X = 東、Y = 上、Z = 北）。
///
/// 1マスは対角線 (x, z)-(x+1, z+1) で三角形2枚に分ける（見た目のメッシュと同じ分け方）。高さはその三角形の面の上で求めるので、
/// 見た目の地面とずれない（4隅をならす補間ではない）。
/// 地形を1枚だけと決めつけない作り（左下の位置と大きさはデータから読む）なので、あとで区画を並べて使える。
///
/// 使い方の例:
///   Terrain terrain;
///   terrain.Load("Resources/terrain/practice_terrain.json"); // 失敗したら止まる
///   float height = terrain.GetHeight(x, z);
///   RaycastHit hit; if (terrain.Raycast(ray, &hit, 100.0f)) { ... }
/// </summary>
class Terrain {
public:
    // 答え合わせの点（Blender で見た目のメッシュにレイを当てて測った高さ）
    struct Checkpoint {
        float x = 0.0f;
        float z = 0.0f;
        float height = 0.0f;
    };

    // 読み込む。失敗したら Logger::Fatal で止まる
    void Load(const std::string& jsonPath);
    // 読み込む。失敗したら false（中身は前のまま）。読み直しなど、失敗しても続けたいとき用
    bool TryLoad(const std::string& jsonPath, std::string* errorMessage = nullptr);

    bool IsLoaded() const { return !heights.empty(); }

    // (x, z) が地形の範囲の中か（端ちょうども中）
    bool Contains(float x, float z) const;
    // (x, z) の地面の高さ。範囲の外なら false
    bool TryGetHeight(float x, float z, float& outHeight) const;
    // (x, z) の地面の高さ。範囲の外は一番近い端の高さ（読み込み前は 0）
    float GetHeight(float x, float z) const;
    // (x, z) の地面の向き（長さ 1、いつも上向き）。範囲の外は一番近い端の向き
    Vector3 GetNormal(float x, float z) const;
    // レイと地面の一番手前の当たり。ray.direction は長さ 1 でなくてよい（distance はメートル）。
    // 法線はいつも上向き（下から当たっても上を向く）
    bool Raycast(const Ray& ray, RaycastHit* outHit = nullptr, float maxDistance = 1e30f) const;

    // --- 当たり判定（地形のコライダー Collider::MakeHeightfield から CollisionManager が使う。直接呼んでもよい） ---
    // outInfo の normal は「相手を地面から離す向き」（相手の位置に normal * depth を足すと離れる）
    // 球と地面。近くの三角形で、球の中心に一番近い点から押し出す。中心が地面の下なら地面の上へ
    bool CollideSphere(const Sphere& sphere, CollisionInfo* outInfo = nullptr) const;
    // 回転しない箱と地面。箱の底の範囲にかかる三角形のうち一番深くめり込んだ物の向きに押し出す
    // （平らな所は真上、坂は坂の向き、崖は横向きに近い。急な面を壁にするかは CollisionManager が決める）。
    // 床と崖に同時に当たっているときは1回では片方だけなので、押し出した後にもう一度呼ぶ（CollisionManager はそうしている）
    bool CollideBox(const AABB& box, CollisionInfo* outInfo = nullptr) const;

    // 地形全体を囲む箱（高さは一番低い所〜一番高い所）
    AABB GetBounds() const;
    // center のまわり（XZ で半径 radius）の三角形の線を DebugDraw で描く（見た目のメッシュと重なるか見る用）
    void DrawDebugGrid(const Vector3& center, float radius, const Vector4& color) const;

    const std::string& GetName() const { return name; }
    int GetSamplesX() const { return samplesX; }
    int GetSamplesZ() const { return samplesZ; }
    float GetCellSize() const { return cellSize; }
    float GetOriginX() const { return originX; }
    float GetOriginZ() const { return originZ; }
    float GetMinHeight() const { return minHeight; }
    float GetMaxHeight() const { return maxHeight; }
    const std::vector<Checkpoint>& GetCheckpoints() const { return checkpoints; }

private:
    // 格子の点 (ix, iz) の位置
    Vector3 GetSamplePoint(int ix, int iz) const;
    // (x, z) が入るマスと、マスの中の位置（0〜1）。範囲の外は端のマスに寄せる
    void FindCell(float x, float z, int& outIx, int& outIz, float& outU, float& outV) const;
    // マス (ix, iz) の三角形2枚とレイの当たり（近い方）
    bool RaycastCell(const Ray& ray, int ix, int iz, RaycastHit* outHit, float maxDistance) const;
    // XZ の範囲にかかるマスの番号の範囲（両端を含む）。範囲が地形の外なら false
    bool GetCellRange(float minX, float minZ, float maxX, float maxZ, int& outIx0, int& outIz0, int& outIx1, int& outIz1) const;

    std::string name;
    int samplesX = 0;
    int samplesZ = 0;
    float cellSize = 1.0f;
    float originX = 0.0f;
    float originZ = 0.0f;
    float minHeight = 0.0f;
    float maxHeight = 0.0f;
    std::vector<float> heights; // index = iz * samplesX + ix
    std::vector<Checkpoint> checkpoints;
};
