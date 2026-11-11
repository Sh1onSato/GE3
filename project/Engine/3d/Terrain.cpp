#include "Terrain.h"
#include "Calculation.h"
#include "Collision.h"
#include "DebugDraw.h"
#include "JsonParser.h"
#include "JsonValue.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
    constexpr float kEpsilon = 1e-6f;
    // 線を地面から少し浮かせる（地面とちらつかないように）
    constexpr float kDebugLineLift = 0.03f;
    // DrawDebugGrid で一度に描くマスの数の上限（片方向）。線が多すぎると重くなる
    constexpr int kMaxDebugCells = 64;
    // レイを切る箱を上下に少し広げる（平らな地形で箱の厚みが 0 にならないように）
    constexpr float kBoundsMargin = 0.01f;

    // JSON から読んだ設定（高さの中身はまだ）
    struct TerrainSettings {
        std::string name;
        int samplesX = 0;
        int samplesZ = 0;
        float cellSize = 0.0f;
        float originX = 0.0f;
        float originZ = 0.0f;
        std::string heightFile;
        std::vector<Terrain::Checkpoint> checkpoints;
    };

    bool ReadNumber(const JsonValue& object, const char* key, double& outValue) {
        const JsonValue& value = object[key];
        if (!value.IsNumber()) {
            return false;
        }
        outValue = value.AsNumber();
        return true;
    }

    // 設定の JSON を読む。足りない・おかしい項目があれば false と理由
    bool ReadSettings(const std::string& jsonPath, TerrainSettings& out, std::string& error) {
        std::ifstream file(jsonPath, std::ios::binary);
        if (!file.is_open()) {
            error = "ファイルを開けません: " + jsonPath;
            return false;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        JsonValue root;
        try {
            root = JsonParser::Parse(ss.str());
        } catch (const std::runtime_error& e) {
            error = std::format("JSON の書き方が間違っています: {} ({})", jsonPath, e.what());
            return false;
        }
        double samplesX = 0.0, samplesZ = 0.0, cellSize = 0.0;
        const JsonValue& origin = root["originXZ"];
        if (!root.IsObject() || !ReadNumber(root, "samplesX", samplesX) || !ReadNumber(root, "samplesZ", samplesZ) ||
            !ReadNumber(root, "cellSize", cellSize) || !origin.IsArray() || origin.Size() != 2 ||
            !origin[size_t{ 0 }].IsNumber() || !origin[size_t{ 1 }].IsNumber() || !root["heightFile"].IsString()) {
            error = "samplesX / samplesZ / cellSize / originXZ / heightFile のどれかが無いか、形が違います: " + jsonPath;
            return false;
        }
        if (samplesX < 2.0 || samplesZ < 2.0 || cellSize <= 0.0) {
            error = std::format("点の数は 2 以上、間隔は 0 より大きくしてください（{} × {}、間隔 {}）: {}", samplesX, samplesZ, cellSize, jsonPath);
            return false;
        }
        out.name = root["name"].IsString() ? root["name"].AsString() : std::filesystem::path(jsonPath).stem().string();
        out.samplesX = static_cast<int>(samplesX);
        out.samplesZ = static_cast<int>(samplesZ);
        out.cellSize = static_cast<float>(cellSize);
        out.originX = static_cast<float>(origin[size_t{ 0 }].AsNumber());
        out.originZ = static_cast<float>(origin[size_t{ 1 }].AsNumber());
        out.heightFile = root["heightFile"].AsString();
        // 答え合わせの点（無くてもよい）
        for (const JsonValue& point : root["checkpoints"].AsArray()) {
            double x = 0.0, z = 0.0, height = 0.0;
            if (ReadNumber(point, "x", x) && ReadNumber(point, "z", z) && ReadNumber(point, "height", height)) {
                out.checkpoints.push_back({ static_cast<float>(x), static_cast<float>(z), static_cast<float>(height) });
            }
        }
        return true;
    }

    // 高さのファイルを読む（float32 リトルエンディアン。x64 の Windows はリトルエンディアンなのでそのまま読める）
    bool ReadHeights(const std::filesystem::path& path, int samplesX, int samplesZ, std::vector<float>& out, std::string& error) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            error = "高さのファイルを開けません: " + path.string();
            return false;
        }
        const std::streamoff fileSize = file.tellg();
        const size_t count = static_cast<size_t>(samplesX) * static_cast<size_t>(samplesZ);
        const std::streamoff expectedSize = static_cast<std::streamoff>(count * sizeof(float));
        if (fileSize != expectedSize) {
            error = std::format("高さのファイルの大きさが合いません（{} バイト、{} × {} × 4 = {} バイトのはず）: {}", fileSize, samplesX, samplesZ,
                                expectedSize, path.string());
            return false;
        }
        out.resize(count);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(out.data()), expectedSize);
        if (!file) {
            error = "高さのファイルを最後まで読めません: " + path.string();
            return false;
        }
        for (float height : out) {
            if (!std::isfinite(height)) {
                error = "高さのファイルに数字でない値（NaN・無限大）があります: " + path.string();
                return false;
            }
        }
        return true;
    }

    // マスの中の位置 (u, v) の高さ。対角線 (0,0)-(1,1) で分けた三角形の面の上
    //   u >= v: 三角形 (0,0) (1,0) (1,1)   u < v: 三角形 (0,0) (1,1) (0,1)
    float HeightInCell(float h00, float h10, float h01, float h11, float u, float v) {
        if (u >= v) {
            return h00 + (h10 - h00) * u + (h11 - h10) * v;
        }
        return h00 + (h11 - h01) * u + (h01 - h00) * v;
    }

    // 箱の中を通る区間 [tEnter, tExit] に切る（direction は長さ 1）。通らなければ false
    bool ClipRayToBox(const Ray& ray, const AABB& box, float& tEnter, float& tExit) {
        const float origin[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
        const float direction[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
        const float boxMin[3] = { box.min.x, box.min.y, box.min.z };
        const float boxMax[3] = { box.max.x, box.max.y, box.max.z };
        for (int axis = 0; axis < 3; ++axis) {
            if (std::fabs(direction[axis]) < kEpsilon) {
                // この軸には進まない。始点が板の外なら当たらない
                if (origin[axis] < boxMin[axis] || origin[axis] > boxMax[axis]) {
                    return false;
                }
                continue;
            }
            float t1 = (boxMin[axis] - origin[axis]) / direction[axis];
            float t2 = (boxMax[axis] - origin[axis]) / direction[axis];
            if (t1 > t2) {
                std::swap(t1, t2);
            }
            tEnter = (std::max)(tEnter, t1);
            tExit = (std::min)(tExit, t2);
            if (tEnter > tExit) {
                return false;
            }
        }
        return true;
    }

    // 三角形の面の向き（長さ 1、いつも上向き）
    Vector3 TriangleNormalUp(const Vector3& a, const Vector3& b, const Vector3& c) {
        Vector3 normal = Calculation::Normalize(Calculation::Cross(b - a, c - a));
        if (normal.y < 0.0f) {
            normal = normal * -1.0f;
        }
        return normal;
    }

    // XZ の点 (x, z) が三角形 abc の中か（辺の上も中）
    bool IsInsideTriangleXZ(float x, float z, const Vector3& a, const Vector3& b, const Vector3& c) {
        auto side = [x, z](const Vector3& p0, const Vector3& p1) {
            return (p1.x - p0.x) * (z - p0.z) - (p1.z - p0.z) * (x - p0.x);
        };
        const float s0 = side(a, b), s1 = side(b, c), s2 = side(c, a);
        const bool hasNegative = s0 < -kEpsilon || s1 < -kEpsilon || s2 < -kEpsilon;
        const bool hasPositive = s0 > kEpsilon || s1 > kEpsilon || s2 > kEpsilon;
        return !(hasNegative && hasPositive);
    }

    // 三角形 abc のうち、XZ の四角 [minX, maxX] × [minZ, maxZ] に入っている部分の一番高い所。重なっていなければ false。
    // 三角形は平面なので、一番高い所は「四角に入っている頂点」「三角形に入っている四角の角」「辺と四角の辺の交わる所」のどれか
    bool MaxHeightOfTriangleInRect(const Vector3& a, const Vector3& b, const Vector3& c,
                                   float minX, float minZ, float maxX, float maxZ, float& outHeight) {
        bool isFound = false;
        auto consider = [&](float height) {
            if (!isFound || height > outHeight) {
                isFound = true;
                outHeight = height;
            }
        };
        const Vector3* vertices[3] = { &a, &b, &c };
        for (const Vector3* vertex : vertices) {
            if (vertex->x >= minX && vertex->x <= maxX && vertex->z >= minZ && vertex->z <= maxZ) {
                consider(vertex->y);
            }
        }
        // 四角の角（三角形の面の高さで）
        const Vector3 normal = TriangleNormalUp(a, b, c);
        if (normal.y > kEpsilon) {
            const float corners[4][2] = { { minX, minZ }, { maxX, minZ }, { minX, maxZ }, { maxX, maxZ } };
            for (const auto& corner : corners) {
                if (IsInsideTriangleXZ(corner[0], corner[1], a, b, c)) {
                    consider(a.y - (normal.x * (corner[0] - a.x) + normal.z * (corner[1] - a.z)) / normal.y);
                }
            }
        }
        // 三角形の辺と四角の辺（x 一定・z 一定の線）の交わる所
        for (int i = 0; i < 3; ++i) {
            const Vector3& p0 = *vertices[i];
            const Vector3& p1 = *vertices[(i + 1) % 3];
            const Vector3 edge = p1 - p0;
            if (std::fabs(edge.x) > kEpsilon) {
                for (const float x : { minX, maxX }) {
                    const float t = (x - p0.x) / edge.x;
                    const float z = p0.z + edge.z * t;
                    if (t >= 0.0f && t <= 1.0f && z >= minZ && z <= maxZ) {
                        consider(p0.y + edge.y * t);
                    }
                }
            }
            if (std::fabs(edge.z) > kEpsilon) {
                for (const float z : { minZ, maxZ }) {
                    const float t = (z - p0.z) / edge.z;
                    const float x = p0.x + edge.x * t;
                    if (t >= 0.0f && t <= 1.0f && x >= minX && x <= maxX) {
                        consider(p0.y + edge.y * t);
                    }
                }
            }
        }
        return isFound;
    }
}

void Terrain::Load(const std::string& jsonPath) {
    std::string error;
    if (!TryLoad(jsonPath, &error)) {
        Logger::Fatal(error);
    }
}

bool Terrain::TryLoad(const std::string& jsonPath, std::string* errorMessage) {
    TerrainSettings settings;
    std::vector<float> newHeights;
    std::string error;
    if (!ReadSettings(jsonPath, settings, error) ||
        !ReadHeights(std::filesystem::path(jsonPath).parent_path() / settings.heightFile, settings.samplesX, settings.samplesZ, newHeights, error)) {
        if (errorMessage) {
            *errorMessage = "Terrain: " + error;
        }
        return false;
    }
    // 全部読めてから入れ替える（途中で失敗しても前の地形が残る）
    name = settings.name;
    samplesX = settings.samplesX;
    samplesZ = settings.samplesZ;
    cellSize = settings.cellSize;
    originX = settings.originX;
    originZ = settings.originZ;
    checkpoints = std::move(settings.checkpoints);
    heights = std::move(newHeights);
    const auto [lowest, highest] = std::minmax_element(heights.begin(), heights.end());
    minHeight = *lowest;
    maxHeight = *highest;
    return true;
}

bool Terrain::Contains(float x, float z) const {
    if (!IsLoaded()) {
        return false;
    }
    const float maxX = originX + static_cast<float>(samplesX - 1) * cellSize;
    const float maxZ = originZ + static_cast<float>(samplesZ - 1) * cellSize;
    return x >= originX && x <= maxX && z >= originZ && z <= maxZ;
}

Vector3 Terrain::GetSamplePoint(int ix, int iz) const {
    return { originX + static_cast<float>(ix) * cellSize, heights[static_cast<size_t>(iz) * samplesX + ix],
             originZ + static_cast<float>(iz) * cellSize };
}

void Terrain::FindCell(float x, float z, int& outIx, int& outIz, float& outU, float& outV) const {
    const float gridX = (x - originX) / cellSize;
    const float gridZ = (z - originZ) / cellSize;
    outIx = std::clamp(static_cast<int>(std::floor(gridX)), 0, samplesX - 2);
    outIz = std::clamp(static_cast<int>(std::floor(gridZ)), 0, samplesZ - 2);
    outU = std::clamp(gridX - static_cast<float>(outIx), 0.0f, 1.0f);
    outV = std::clamp(gridZ - static_cast<float>(outIz), 0.0f, 1.0f);
}

bool Terrain::TryGetHeight(float x, float z, float& outHeight) const {
    if (!Contains(x, z)) {
        return false;
    }
    outHeight = GetHeight(x, z);
    return true;
}

float Terrain::GetHeight(float x, float z) const {
    if (!IsLoaded()) {
        return 0.0f;
    }
    int ix = 0, iz = 0;
    float u = 0.0f, v = 0.0f;
    FindCell(x, z, ix, iz, u, v);
    const size_t row = static_cast<size_t>(iz) * samplesX;
    const size_t nextRow = row + samplesX;
    return HeightInCell(heights[row + ix], heights[row + ix + 1], heights[nextRow + ix], heights[nextRow + ix + 1], u, v);
}

Vector3 Terrain::GetNormal(float x, float z) const {
    if (!IsLoaded()) {
        return { 0.0f, 1.0f, 0.0f };
    }
    int ix = 0, iz = 0;
    float u = 0.0f, v = 0.0f;
    FindCell(x, z, ix, iz, u, v);
    const size_t row = static_cast<size_t>(iz) * samplesX;
    const size_t nextRow = row + samplesX;
    const float h00 = heights[row + ix], h10 = heights[row + ix + 1];
    const float h01 = heights[nextRow + ix], h11 = heights[nextRow + ix + 1];
    // 三角形の面の傾き（1マス進んだときの高さの変わり方）
    const float slopeU = (u >= v) ? (h10 - h00) : (h11 - h01);
    const float slopeV = (u >= v) ? (h11 - h10) : (h01 - h00);
    // Normalize は Vector3 用と Quaternion 用があるので、型を書いて選ぶ（{...} だけだと決められない）
    return Calculation::Normalize(Vector3{ -slopeU / cellSize, 1.0f, -slopeV / cellSize });
}

bool Terrain::RaycastCell(const Ray& ray, int ix, int iz, RaycastHit* outHit, float maxDistance) const {
    const Vector3 p00 = GetSamplePoint(ix, iz);
    const Vector3 p10 = GetSamplePoint(ix + 1, iz);
    const Vector3 p01 = GetSamplePoint(ix, iz + 1);
    const Vector3 p11 = GetSamplePoint(ix + 1, iz + 1);
    RaycastHit hitA{}, hitB{};
    const bool isHitA = Collision::RayTriangle(ray, p00, p10, p11, &hitA, maxDistance);
    const bool isHitB = Collision::RayTriangle(ray, p00, p11, p01, &hitB, maxDistance);
    if (!isHitA && !isHitB) {
        return false;
    }
    RaycastHit hit = (isHitA && (!isHitB || hitA.distance <= hitB.distance)) ? hitA : hitB;
    // RayTriangle は裏からも当たり、法線の上下は頂点の順番で決まるので、いつも上を向ける
    if (hit.normal.y < 0.0f) {
        hit.normal = hit.normal * -1.0f;
    }
    if (outHit) {
        *outHit = hit;
    }
    return true;
}

bool Terrain::Raycast(const Ray& ray, RaycastHit* outHit, float maxDistance) const {
    const float length = Calculation::Length(ray.direction);
    if (!IsLoaded() || length < kEpsilon) {
        return false;
    }
    const Ray unitRay = { ray.origin, ray.direction * (1.0f / length) };
    float tEnter = 0.0f;
    float tExit = maxDistance;
    if (!ClipRayToBox(unitRay, GetBounds(), tEnter, tExit)) {
        return false;
    }
    // 箱に入った所のマスから、レイが通るマスを近い順にたどる（2D の DDA）。最初に当たったマスの当たりが一番手前
    const Vector3 start = unitRay.origin + unitRay.direction * tEnter;
    int ix = 0, iz = 0;
    float u = 0.0f, v = 0.0f;
    FindCell(start.x, start.z, ix, iz, u, v);
    const Vector3& direction = unitRay.direction;
    const int stepX = direction.x > 0.0f ? 1 : -1;
    const int stepZ = direction.z > 0.0f ? 1 : -1;
    constexpr float kInfinity = std::numeric_limits<float>::infinity();
    float tNextX = kInfinity, tDeltaX = kInfinity, tNextZ = kInfinity, tDeltaZ = kInfinity;
    if (std::fabs(direction.x) > kEpsilon) {
        const float boundaryX = originX + static_cast<float>(ix + (stepX > 0 ? 1 : 0)) * cellSize;
        tNextX = tEnter + (boundaryX - start.x) / direction.x;
        tDeltaX = cellSize / std::fabs(direction.x);
    }
    if (std::fabs(direction.z) > kEpsilon) {
        const float boundaryZ = originZ + static_cast<float>(iz + (stepZ > 0 ? 1 : 0)) * cellSize;
        tNextZ = tEnter + (boundaryZ - start.z) / direction.z;
        tDeltaZ = cellSize / std::fabs(direction.z);
    }
    const int maxSteps = samplesX + samplesZ;
    for (int step = 0; step <= maxSteps; ++step) {
        if (RaycastCell(unitRay, ix, iz, outHit, tExit)) {
            return true;
        }
        const bool isStepX = tNextX < tNextZ;
        if ((isStepX ? tNextX : tNextZ) > tExit) {
            break;
        }
        if (isStepX) {
            ix += stepX;
            tNextX += tDeltaX;
        } else {
            iz += stepZ;
            tNextZ += tDeltaZ;
        }
        if (ix < 0 || iz < 0 || ix > samplesX - 2 || iz > samplesZ - 2) {
            break;
        }
    }
    return false;
}

bool Terrain::GetCellRange(float minX, float minZ, float maxX, float maxZ, int& outIx0, int& outIz0, int& outIx1, int& outIz1) const {
    if (!IsLoaded()) {
        return false;
    }
    const float terrainMaxX = originX + static_cast<float>(samplesX - 1) * cellSize;
    const float terrainMaxZ = originZ + static_cast<float>(samplesZ - 1) * cellSize;
    if (maxX < originX || minX > terrainMaxX || maxZ < originZ || minZ > terrainMaxZ) {
        return false;
    }
    auto toCell = [this](float value, float origin, int samples) {
        return std::clamp(static_cast<int>(std::floor((value - origin) / cellSize)), 0, samples - 2);
    };
    outIx0 = toCell(minX, originX, samplesX);
    outIx1 = toCell(maxX, originX, samplesX);
    outIz0 = toCell(minZ, originZ, samplesZ);
    outIz1 = toCell(maxZ, originZ, samplesZ);
    return true;
}

bool Terrain::CollideSphere(const Sphere& sphere, CollisionInfo* outInfo) const {
    const Vector3& center = sphere.center;
    const float radius = sphere.radius;
    int ix0 = 0, iz0 = 0, ix1 = 0, iz1 = 0;
    if (!GetCellRange(center.x - radius, center.z - radius, center.x + radius, center.z + radius, ix0, iz0, ix1, iz1)) {
        return false;
    }
    // 中心が地面の下（速く落ちて突き抜けかけた）なら、地面の上へ押し出す
    float ground = 0.0f;
    if (TryGetHeight(center.x, center.z, ground) && center.y < ground) {
        if (outInfo) {
            const Vector3 normal = GetNormal(center.x, center.z);
            outInfo->normal = normal;
            outInfo->depth = (ground - center.y) * normal.y + radius;
        }
        return true;
    }
    // 近くの三角形のうち、中心に一番近い点が半径より近い物（一番近い = 一番深くめり込んでいる）
    bool isHit = false;
    float closestDistance = radius;
    Vector3 hitNormal = { 0.0f, 1.0f, 0.0f };
    for (int iz = iz0; iz <= iz1; ++iz) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            const Vector3 p00 = GetSamplePoint(ix, iz), p10 = GetSamplePoint(ix + 1, iz);
            const Vector3 p01 = GetSamplePoint(ix, iz + 1), p11 = GetSamplePoint(ix + 1, iz + 1);
            const Vector3 triangles[2][3] = { { p00, p10, p11 }, { p00, p11, p01 } };
            for (const auto& triangle : triangles) {
                const Vector3 closest = Collision::ClosestPointOnTriangle(center, triangle[0], triangle[1], triangle[2]);
                const Vector3 toCenter = center - closest;
                const float distance = Calculation::Length(toCenter);
                if (distance >= closestDistance) {
                    continue;
                }
                isHit = true;
                closestDistance = distance;
                // 中心が面の上ちょうどなら、面の向きに押す
                hitNormal = distance > kEpsilon ? toCenter * (1.0f / distance) : TriangleNormalUp(triangle[0], triangle[1], triangle[2]);
            }
        }
    }
    if (isHit && outInfo) {
        outInfo->normal = hitNormal;
        outInfo->depth = radius - closestDistance;
    }
    return isHit;
}

bool Terrain::CollideBox(const AABB& box, CollisionInfo* outInfo) const {
    int ix0 = 0, iz0 = 0, ix1 = 0, iz1 = 0;
    if (!GetCellRange(box.min.x, box.min.z, box.max.x, box.max.z, ix0, iz0, ix1, iz1)) {
        return false;
    }
    // 箱の底の範囲にかかる三角形ごとに、その三角形の一番高い所が箱の底よりどれだけ上か（= 面から底の角までのめり込み）を調べ、
    // 一番深い三角形の向きで押し出す。床と崖の両方に当たっているときは、CollisionManager が押し出した後にもう一度調べて残りを押す
    bool isHit = false;
    float deepest = 0.0f;
    Vector3 hitNormal = { 0.0f, 1.0f, 0.0f };
    for (int iz = iz0; iz <= iz1; ++iz) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            const Vector3 p00 = GetSamplePoint(ix, iz), p10 = GetSamplePoint(ix + 1, iz);
            const Vector3 p01 = GetSamplePoint(ix, iz + 1), p11 = GetSamplePoint(ix + 1, iz + 1);
            const Vector3 triangles[2][3] = { { p00, p10, p11 }, { p00, p11, p01 } };
            for (const auto& triangle : triangles) {
                float top = 0.0f;
                if (!MaxHeightOfTriangleInRect(triangle[0], triangle[1], triangle[2], box.min.x, box.min.z, box.max.x, box.max.z, top) ||
                    top <= box.min.y) {
                    continue;
                }
                const Vector3 normal = TriangleNormalUp(triangle[0], triangle[1], triangle[2]);
                // 高さの差を、面に垂直な距離に直す（その分だけ面の向きに押すと、底の角が面の上に出る）
                const float depth = (top - box.min.y) * normal.y;
                if (depth > deepest) {
                    isHit = true;
                    deepest = depth;
                    hitNormal = normal;
                }
            }
        }
    }
    if (isHit && outInfo) {
        outInfo->normal = hitNormal;
        outInfo->depth = deepest;
    }
    return isHit;
}

AABB Terrain::GetBounds() const {
    return { { originX, minHeight - kBoundsMargin, originZ },
             { originX + static_cast<float>(samplesX - 1) * cellSize, maxHeight + kBoundsMargin,
               originZ + static_cast<float>(samplesZ - 1) * cellSize } };
}

void Terrain::DrawDebugGrid(const Vector3& center, float radius, const Vector4& color) const {
    if (!IsLoaded()) {
        return;
    }
    const int cellRadius = (std::min)(static_cast<int>(std::ceil(radius / cellSize)), kMaxDebugCells / 2);
    int centerIx = 0, centerIz = 0;
    float u = 0.0f, v = 0.0f;
    FindCell(center.x, center.z, centerIx, centerIz, u, v);
    const int ix0 = (std::max)(centerIx - cellRadius, 0), ix1 = (std::min)(centerIx + cellRadius, samplesX - 2);
    const int iz0 = (std::max)(centerIz - cellRadius, 0), iz1 = (std::min)(centerIz + cellRadius, samplesZ - 2);
    const Vector3 lift = { 0.0f, kDebugLineLift, 0.0f };
    for (int iz = iz0; iz <= iz1; ++iz) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            const Vector3 p00 = GetSamplePoint(ix, iz) + lift;
            const Vector3 p10 = GetSamplePoint(ix + 1, iz) + lift;
            const Vector3 p01 = GetSamplePoint(ix, iz + 1) + lift;
            const Vector3 p11 = GetSamplePoint(ix + 1, iz + 1) + lift;
            DebugDraw::Line(p00, p10, color);
            DebugDraw::Line(p00, p01, color);
            DebugDraw::Line(p00, p11, color); // 対角線
            if (ix == ix1) {
                DebugDraw::Line(p10, p11, color);
            }
            if (iz == iz1) {
                DebugDraw::Line(p01, p11, color);
            }
        }
    }
}
