#pragma once
#include <string>
#include <vector>
#include <stdint.h>

// --- 基本構造体 ---

struct Vector2 {
	float x, y;

	Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }
	Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
	Vector2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
};

struct Vector3 {
	float x, y, z;

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
	Vector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
	Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
	Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
	Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
};

struct Vector4 {
	float x, y, z, w;
};

struct Matrix3x3 {
	float m[3][3];
};

struct Matrix4x4 {
	float m[4][4];

	Matrix4x4 operator+(const Matrix4x4& other) const;
	Matrix4x4 operator-(const Matrix4x4& other) const;
	Matrix4x4 operator*(const Matrix4x4& other) const;
};

struct Quaternion {
	float x, y, z, w;
};

// --- 形状・衝突用構造体 ---

struct Sphere {
	Vector3 center;
	float radius;
};

struct Ray {
	Vector3 origin;
	Vector3 direction;
};

struct AABB {
	Vector3 min;
	Vector3 max;
};

struct RaycastHit {
	Vector3 hitPoint; // 衝突地点
	float distance;   // 始点からの距離
	Vector3 normal;   // 法線
};

// --- エンジン用構造体 ---

struct Transform {
	Vector3 scale,
		rotate,
		translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float dissolveHeight;    // 実体化演出：このワールドY座標より上のピクセルを描かない
	float dissolveEdgeWidth; // 実体化演出：境界の発光幅。0以下で演出無効
	float dissolveTime;      // 実体化演出：ノイズを動かすための経過時間[秒]
	Matrix4x4 uvTransform;
	float dissolveBlockStrength; // 実体化演出：ブロック欠けの強さ（0～1）。全体が見えた時点で0にする
	float padding[3];
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct PointLight {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;
	float decay;
	float padding[2];
};
static_assert(sizeof(PointLight) == 48);

struct SpotLight {
	Vector4 color;
	Vector3 position;
	float intensity;
	Vector3 direction;
	float distance;
	float decay;
	float cosAngle;
	float cosFalloffStart;
	float padding;
};
static_assert(sizeof(SpotLight) == 64);

struct ShadowData {
	Matrix4x4 lightViewProjection;
	float bias;
	float padding[3];
};
static_assert(sizeof(ShadowData) == 80);

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
