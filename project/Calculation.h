#pragma once
#include <cmath>
#include <assert.h>

class Calculation {
public:

	struct Vector3 {
		float x, y, z;
	};

	struct Matrix3x3 {
		float m[3][3];
	};

	struct Matrix4x4 {
		float m[4][4];
	};

	struct Sphere {
		Vector3 center;
		float radius;
	};

	Vector3 translate = {};
	Vector3 scale = {};
	Vector3 rotate = {};
	Vector3 point = {};

	Matrix4x4 m1 = {};
	Matrix4x4 m2 = {};
	Matrix4x4 transformationMatrix = {};

	static const int kColumnWidth = 1280;
	static const int kRowHeight = 720;
	// 加算
	Vector3 Add(const Vector3& a, const Vector3& b);
	// 減算
	Vector3 Subtract(const Vector3& a, const Vector3& b);
	// スカラー倍
	Vector3 Multiply(float b, const Vector3& a);
	// 内積
	float Dot(const Vector3& a, const Vector3& b);
	// 長さ(ノルム)
	float Length(const Vector3& a);
	// 正規化
	Vector3 Normalize(Vector3& a);


	//１.行列の加法
	Matrix4x4 Add(const Matrix4x4& a, const Matrix4x4& b);
	//２.行列の減法
	Matrix4x4 Subtract(const Matrix4x4& a, const Matrix4x4& b);
	//３.行列の積
	Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);
	//４.逆行列
	Matrix4x4 Inverse(const Matrix4x4& a);
	//５.転倒行列
	Matrix4x4 Transpose(const Matrix4x4& a);
	//６.単項行列の作成
	Matrix4x4 MakeIdentity4x4();

	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	//平行移動行列
	Matrix4x4 MakeTranslationMatrix(const Vector3& translate);

	//座標変換行列
	Vector3 Transform(const Vector3& vector, Matrix4x4& matrix);

	//1.X軸回転行列
	Matrix4x4 MakeRotationXMatrix(float radian);
	//2.Y軸回転行列
	Matrix4x4 MakeRotationYMatrix(float radian);
	//3.Z軸回転行列
	Matrix4x4 MakeRotationZMatrix(float radian);

	// 3次元アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	//1. 遠視投影行列
	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farCrip);
	//2. 正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
	//3. ビューポート変換行列
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);


	void DrawSphere(const Sphere& sphere, Matrix4x4& viewProjectionMatrix, Matrix4x4& viewportMatrix, uint32_t color);

	void DrawGrid(Matrix4x4& viewProjectionMatrix,Matrix4x4& viewportMatrix);
};

