#pragma once
#include <cmath>
#include <assert.h>
#include <stdint.h>

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

struct Sphere {
	Vector3 center;
	float radius;
};

struct Quaternion {
	float x, y, z, w;
};

class Calculation {
public:
	// インスタンス化を禁止
	Calculation() = delete;

	static const int kColumnWidth = 1280;
	static const int kRowHeight = 720;

	// ベクトル演算
	static Vector3 Add(const Vector3& a, const Vector3& b) { return a + b; }
	static Vector3 Subtract(const Vector3& a, const Vector3& b) { return a - b; }
	static Vector3 Multiply(float b, const Vector3& a) { return a * b; }
	static float Dot(const Vector3& a, const Vector3& b);
	static float Length(const Vector3& a);
	static Vector3 Normalize(const Vector3& a);

	// 行列演算
	static Matrix4x4 Add(const Matrix4x4& a, const Matrix4x4& b) { return a + b; }
	static Matrix4x4 Subtract(const Matrix4x4& a, const Matrix4x4& b) { return a - b; }
	static Matrix4x4 MatrixMultiply(const Matrix4x4& a, const Matrix4x4& b) { return a * b; }
	static Matrix4x4 Inverse(const Matrix4x4& a);
	static Matrix4x4 Transpose(const Matrix4x4& a);
	static Matrix4x4 MakeIdentity4x4();

	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);
	static Matrix4x4 MakeTranslationMatrix(const Vector3& translate);
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

	static Matrix4x4 MakeRotationXMatrix(float radian);
	static Matrix4x4 MakeRotationYMatrix(float radian);
	static Matrix4x4 MakeRotationZMatrix(float radian);

	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);

	static Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& viewMatrix);

	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farCrip);
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
	static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

	// クォータニオン演算
	static Quaternion Multiply(const Quaternion& q, const Quaternion& r);
	static Quaternion IdentityQuaternion();
	static Quaternion Conjugate(const Quaternion& q);
	static float Norm(const Quaternion& q);
	static Quaternion Normalize(const Quaternion& q);
	static Quaternion Inverse(const Quaternion& q);
	static Quaternion MakeAxisAngleQuaternion(const Vector3& axis, float angle);
	static Matrix4x4 MakeRotateMatrix(const Quaternion& q);
	static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);

	// デバッグ用描画（実際の実装は描画クラスで行うべきだが、現状維持）
	static void DrawSphere(const Sphere& sphere, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);
	static void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix);
};

