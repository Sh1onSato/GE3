#include "Calculation.h"
#include <cmath> 
#include <numbers> 

Calculation::Vector3 Calculation::Add(const Vector3& a, const Vector3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Calculation::Vector3 Calculation::Subtract(const Vector3& a, const Vector3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Calculation::Vector3 Calculation::Multiply(float b, const Vector3& a) {
	return { a.x * b, a.y * b, a.z * b };
}
float Calculation::Dot(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Calculation::Length(const Vector3& a) {
	return std::sqrt(Dot(a, a));
}

Calculation::Vector3 Calculation::Normalize(Vector3& a) {
	float length = Length(a);
	if (length == 0) {
		return { 0, 0, 0 };
	}
	return { a.x / length, a.y / length, a.z / length };
}

Calculation::Matrix4x4 Calculation::Add(const Matrix4x4& a, const Matrix4x4& b)
{
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = a.m[i][j] + b.m[i][j];
		}
	}
	return result;
}

Calculation::Matrix4x4 Calculation::Subtract(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = a.m[i][j] - b.m[i][j];
		}
	}
	return result;
}

Calculation::Matrix4x4 Calculation::Multiply(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = 0;
			for (int k = 0; k < 4; ++k) {
				result.m[i][j] += a.m[i][k] * b.m[k][j];
			}
		}
	}
	return result;
}

Calculation::Matrix4x4 Calculation::Inverse(const Matrix4x4& a) {
	Matrix4x4 result;
	// 逆行列の計算を実装する
	float det = 
		a.m[0][3] * a.m[1][2] * a.m[2][1] * a.m[3][0] - a.m[0][2] * a.m[1][3] * a.m[2][1] * a.m[3][0] -
		a.m[0][3] * a.m[1][1] * a.m[2][2] * a.m[3][0] + a.m[0][1] * a.m[1][3] * a.m[2][2] * a.m[3][0] +
		a.m[0][2] * a.m[1][1] * a.m[2][3] * a.m[3][0] - a.m[0][1] * a.m[1][2] * a.m[2][3] * a.m[3][0] -
		a.m[0][3] * a.m[1][2] * a.m[2][0] * a.m[3][1] + a.m[0][2] * a.m[1][3] * a.m[2][0] * a.m[3][1] +
		a.m[0][3] * a.m[1][0] * a.m[2][2] * a.m[3][1] - a.m[0][0] * a.m[1][3] * a.m[2][2] * a.m[3][1] -
		a.m[0][2] * a.m[1][0] * a.m[2][3] * a.m[3][1] + a.m[0][0] * a.m[1][2] * a.m[2][3] * a.m[3][1] +
		a.m[0][3] * a.m[1][1] * a.m[2][0] * a.m[3][2] - a.m[0][1] * a.m[1][3] * a.m[2][0] * a.m[3][2] -
		a.m[0][3] * a.m[1][0] * a.m[2][1] * a.m[3][2] + a.m[0][0] * a.m[1][3] * a.m[2][1] * a.m[3][2] +
		a.m[0][1] * a.m[1][0] * a.m[2][3] * a.m[3][2] - a.m[0][0] * a.m[1][1] * a.m[2][3] * a.m[3][2] -
		a.m[0][2] * a.m[1][1] * a.m[2][0] * a.m[3][3] + a.m[0][1] * a.m[1][2] * a.m[2][0] * a.m[3][3] +
		a.m[0][2] * a.m[1][0] * a.m[2][1] * a.m[3][3] - a.m[0][0] * a.m[1][2] * a.m[2][1] * a.m[3][3] -
		a.m[0][1] * a.m[1][0] * a.m[2][2] * a.m[3][3] + a.m[0][0] * a.m[1][1] * a.m[2][2] * a.m[3][3];

	if (det == 0.0f) {
		return result = {};
	}
	float invDet = 1.0f / det;

	result.m[0][0] = invDet * (
		a.m[1][2] * a.m[2][3] * a.m[3][1] - a.m[1][3] * a.m[2][2] * a.m[3][1] +
		a.m[1][3] * a.m[2][1] * a.m[3][2] - a.m[1][1] * a.m[2][3] * a.m[3][2] -
		a.m[1][2] * a.m[2][1] * a.m[3][3] + a.m[1][1] * a.m[2][2] * a.m[3][3]);

	result.m[0][1] = invDet * (
		a.m[0][3] * a.m[2][2] * a.m[3][1] - a.m[0][2] * a.m[2][3] * a.m[3][1] -
		a.m[0][3] * a.m[2][1] * a.m[3][2] + a.m[0][1] * a.m[2][3] * a.m[3][2] +
		a.m[0][2] * a.m[2][1] * a.m[3][3] - a.m[0][1] * a.m[2][2] * a.m[3][3]);

	result.m[0][2] = invDet * (
		a.m[0][2] * a.m[1][3] * a.m[3][1] - a.m[0][3] * a.m[1][2] * a.m[3][1] +
		a.m[0][3] * a.m[1][1] * a.m[3][2] - a.m[0][1] * a.m[1][3] * a.m[3][2] -
		a.m[0][2] * a.m[1][1] * a.m[3][3] + a.m[0][1] * a.m[1][2] * a.m[3][3]);

	result.m[0][3] = invDet * (
		a.m[0][3] * a.m[1][2] * a.m[2][1] - a.m[0][2] * a.m[1][3] * a.m[2][1] -
		a.m[0][3] * a.m[1][1] * a.m[2][2] + a.m[0][1] * a.m[1][3] * a.m[2][2] +
		a.m[0][2] * a.m[1][1] * a.m[2][3] - a.m[0][1] * a.m[1][2] * a.m[2][3]);

	result.m[1][0] = invDet * (
		a.m[1][3] * a.m[2][2] * a.m[3][0] - a.m[1][2] * a.m[2][3] * a.m[3][0] -
		a.m[1][3] * a.m[2][0] * a.m[3][2] + a.m[1][0] * a.m[2][3] * a.m[3][2] +
		a.m[1][2] * a.m[2][0] * a.m[3][3] - a.m[1][0] * a.m[2][2] * a.m[3][3]);

	result.m[1][1] = invDet * (
		a.m[0][2] * a.m[2][3] * a.m[3][0] - a.m[0][3] * a.m[2][2] * a.m[3][0] +
		a.m[0][3] * a.m[2][0] * a.m[3][2] - a.m[0][0] * a.m[2][3] * a.m[3][2] -
		a.m[0][2] * a.m[2][0] * a.m[3][3] + a.m[0][0] * a.m[2][2] * a.m[3][3]);

	result.m[1][2] = invDet * (
		a.m[0][3] * a.m[1][2] * a.m[3][0] - a.m[0][2] * a.m[1][3] * a.m[3][0] -
		a.m[0][3] * a.m[1][0] * a.m[3][2] + a.m[0][0] * a.m[1][3] * a.m[3][2] +
		a.m[0][2] * a.m[1][0] * a.m[3][3] - a.m[0][0] * a.m[1][2] * a.m[3][3]);

	result.m[1][3] = invDet * (
		a.m[0][2] * a.m[1][3] * a.m[2][0] - a.m[0][3] * a.m[1][2] * a.m[2][0] +
		a.m[0][3] * a.m[1][0] * a.m[2][2] - a.m[0][0] * a.m[1][3] * a.m[2][2] -
		a.m[0][2] * a.m[1][0] * a.m[2][3] + a.m[0][0] * a.m[1][2] * a.m[2][3]);

	result.m[2][0] = invDet * (
		a.m[1][1] * a.m[2][3] * a.m[3][0] - a.m[1][3] * a.m[2][1] * a.m[3][0] +
		a.m[1][3] * a.m[2][0] * a.m[3][1] - a.m[1][0] * a.m[2][3] * a.m[3][1] -
		a.m[1][1] * a.m[2][0] * a.m[3][3] + a.m[1][0] * a.m[2][1] * a.m[3][3]);

	result.m[2][1] = invDet * (
		a.m[0][3] * a.m[2][1] * a.m[3][0] - a.m[0][1] * a.m[2][3] * a.m[3][0] -
		a.m[0][3] * a.m[2][0] * a.m[3][1] + a.m[0][0] * a.m[2][3] * a.m[3][1] +
		a.m[0][1] * a.m[2][0] * a.m[3][3] - a.m[0][0] * a.m[2][1] * a.m[3][3]);

	result.m[2][2] = invDet * (
		a.m[0][1] * a.m[1][3] * a.m[3][0] - a.m[0][3] * a.m[1][1] * a.m[3][0] +
		a.m[0][3] * a.m[1][0] * a.m[3][1] - a.m[0][0] * a.m[1][3] * a.m[3][1] -
		a.m[0][1] * a.m[1][0] * a.m[3][3] + a.m[0][0] * a.m[1][1] * a.m[3][3]);

	result.m[2][3] = invDet * (
		a.m[0][3] * a.m[1][1] * a.m[2][0] - a.m[0][1] * a.m[1][3] * a.m[2][0] -
		a.m[0][3] * a.m[1][0] * a.m[2][1] + a.m[0][0] * a.m[1][3] * a.m[2][1] +
		a.m[0][1] * a.m[1][0] * a.m[2][3] - a.m[0][0] * a.m[1][1] * a.m[2][3]);

	result.m[3][0] = invDet * (
		a.m[1][2] * a.m[2][1] * a.m[3][0] - a.m[1][1] * a.m[2][2] * a.m[3][0] -
		a.m[1][2] * a.m[2][0] * a.m[3][1] + a.m[1][0] * a.m[2][2] * a.m[3][1] +
		a.m[1][1] * a.m[2][0] * a.m[3][2] - a.m[1][0] * a.m[2][1] * a.m[3][2]);

	result.m[3][1] = invDet * (
		a.m[0][1] * a.m[2][2] * a.m[3][0] - a.m[0][2] * a.m[2][1] * a.m[3][0] +
		a.m[0][2] * a.m[2][0] * a.m[3][1] - a.m[0][0] * a.m[2][2] * a.m[3][1] -
		a.m[0][1] * a.m[2][0] * a.m[3][2] + a.m[0][0] * a.m[2][1] * a.m[3][2]);

	result.m[3][2] = invDet * (
		a.m[0][2] * a.m[1][1] * a.m[3][0] - a.m[0][1] * a.m[1][2] * a.m[3][0] -
		a.m[0][2] * a.m[1][0] * a.m[3][1] + a.m[0][0] * a.m[1][2] * a.m[3][1] +
		a.m[0][1] * a.m[1][0] * a.m[3][2] - a.m[0][0] * a.m[1][1] * a.m[3][2]);

	result.m[3][3] = invDet * (
		a.m[0][1] * a.m[1][2] * a.m[2][0] - a.m[0][2] * a.m[1][1] * a.m[2][0] +
		a.m[0][2] * a.m[1][0] * a.m[2][1] - a.m[0][0] * a.m[1][2] * a.m[2][1] -
		a.m[0][1] * a.m[1][0] * a.m[2][2] + a.m[0][0] * a.m[1][1] * a.m[2][2]);

	return result;

}

Calculation::Matrix4x4 Calculation::Transpose(const Matrix4x4& a) {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = a.m[j][i];
		}
	}
	return result;
}

Calculation::Matrix4x4 Calculation::MakeIdentity4x4() {
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (i == j) {
				result.m[i][j] = 1.0f;
			}
			else {
				result.m[i][j] = 0.0f;
			}
		}
	}
	return result;
}

Calculation::Matrix4x4 Calculation::MakeScaleMatrix(const Vector3& Scale) {
	Matrix4x4 result;
	result.m[0][0] = Scale.x;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f;
	result.m[1][1] = Scale.y;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = Scale.z;
	result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Calculation::Matrix4x4 Calculation::MakeTranslationMatrix(const Vector3& Translate) {
	Matrix4x4 result;
	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;
	result.m[3][0] = Translate.x;
	result.m[3][1] = Translate.y;
	result.m[3][2] = Translate.z;
	result.m[3][3] = 1.0f;

	return result;
}

Calculation::Vector3 Calculation::Transform(const Vector3& vector, Matrix4x4& matrix) {
	Vector3 result;
	// 行列とベクトルの積を計算
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];

	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;

	return result;
}

Calculation::Matrix4x4 Calculation::MakeRotationXMatrix(float radian)
{
	Matrix4x4 result;

	float cosRadian = std::cosf(radian);
	float sinRadian = std::sinf(radian);

	result.m[0][0] = 1.0f;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = cosRadian;
	result.m[1][2] = sinRadian;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = -sinRadian;
	result.m[2][2] = cosRadian;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Calculation::Matrix4x4 Calculation::MakeRotationYMatrix(float radian) {
	Matrix4x4 result;

	float cosRadian = std::cosf(radian);
	float sinRadian = std::sinf(radian);

	result.m[0][0] = cosRadian;
	result.m[0][1] = 0.0f;
	result.m[0][2] = -sinRadian;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = 1.0f;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = sinRadian;
	result.m[2][1] = 0.0f;
	result.m[2][2] = cosRadian;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Calculation::Matrix4x4 Calculation::MakeRotationZMatrix(float radian) {
	Matrix4x4 result;

	float cosRadian = std::cosf(radian);
	float sinRadian = std::sinf(radian);

	result.m[0][0] = cosRadian;
	result.m[0][1] = sinRadian;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = -sinRadian;
	result.m[1][1] = cosRadian;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f;
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	// 行列を返す
	return result;
}

Calculation::Matrix4x4 Calculation::MakeAffineMatrix(const Vector3& Scale, const Vector3& Rotate, const Vector3& Translate) {
	Matrix4x4 result;

	Matrix4x4 scaleMatrix = MakeScaleMatrix(Scale);
	Matrix4x4 rotateXMatrix = MakeRotationXMatrix(Rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotationYMatrix(Rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotationZMatrix(Rotate.z);
	Matrix4x4 translationMatrix = MakeTranslationMatrix(Translate);

	result = Multiply(scaleMatrix, rotateXMatrix);
	result = Multiply(result, rotateYMatrix);
	result = Multiply(result, rotateZMatrix);
	result = Multiply(result, translationMatrix);

	// 結果を返す
	return result;
}

Calculation::Matrix4x4 Calculation::MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farCrip) {  
Matrix4x4 result;  

float tanHalfFovY = std::tan(fovY / 2);  

result.m[0][0] = 1.0f / (aspectRatio * tanHalfFovY);  
result.m[0][1] = 0.0f;  
result.m[0][2] = 0.0f;  
result.m[0][3] = 0.0f;  

result.m[1][0] = 0.0f;  
result.m[1][1] = 1.0f / tanHalfFovY;  
result.m[1][2] = 0.0f;  
result.m[1][3] = 0.0f;  

result.m[2][0] = 0.0f;  
result.m[2][1] = 0.0f;  
result.m[2][2] = farCrip / (farCrip - nearClip);  
result.m[2][3] = 1.0f;  

result.m[3][0] = 0.0f;  
result.m[3][1] = 0.0f;  
result.m[3][2] = (-nearClip * farCrip) / (farCrip - nearClip);  
result.m[3][3] = 0.0f;  

return result;  
}

Calculation::Matrix4x4 Calculation::MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result;

	result.m[0][0] = (2 / (right - left));
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = (2 / (top - bottom));
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[2][3] = 0.0f;

	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);
	result.m[3][3] = 1.0f;


	return result;
}

Calculation::Matrix4x4 Calculation::MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	Matrix4x4 result;

	result.m[0][0] = width / 2;
	result.m[0][1] = 0.0f;
	result.m[0][2] = 0.0f;
	result.m[0][3] = 0.0f;

	result.m[1][0] = 0.0f;
	result.m[1][1] = -height / 2;
	result.m[1][2] = 0.0f;
	result.m[1][3] = 0.0f;

	result.m[2][0] = 0.0f;
	result.m[2][1] = 0.0f;
	result.m[2][2] = maxDepth - minDepth;
	result.m[2][3] = 0.0f;

	result.m[3][0] = left + (width / 2);
	result.m[3][1] = top + (height / 2);
	result.m[3][2] = minDepth;
	result.m[3][3] = 1.0f;


	return result;
}

void Calculation::DrawSphere(const Sphere& sphere, Matrix4x4& viewProjectionMatrix,  Matrix4x4& viewportMatrix, uint32_t color) {
	const uint32_t kSubdivisions = 16;
	const float kLonEvery = float(2 * std::numbers::pi) / kSubdivisions;
	const float kLotEvery = float(std::numbers::pi) / kSubdivisions;
	for (uint32_t lotIndex = 0; lotIndex < kLotEvery; ++lotIndex) {
		float lat = float(-std::numbers::pi) / 2 + lotIndex * kLotEvery;
		for (uint32_t lonIndex = 0; lonIndex < kSubdivisions; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			Vector3 a, b, c;
			a = {
			   sphere.center.x + sphere.radius * cosf(lat) * cosf(lon),
			   sphere.center.y + sphere.radius * sinf(lat),
			   sphere.center.z + sphere.radius * cosf(lat) * sinf(lon)
			};

			b = {
				sphere.center.x + sphere.radius * cosf(lat + kLotEvery) * cosf(lon),
				sphere.center.y + sphere.radius * sinf(lat + kLotEvery),
				sphere.center.z + sphere.radius * cosf(lat + kLotEvery) * sinf(lon)
			};

			c = {
				sphere.center.x + sphere.radius * cosf(lat) * cosf(lon + kLonEvery),
				sphere.center.y + sphere.radius * sinf(lat),
				sphere.center.z + sphere.radius * cosf(lat) * sinf(lon + kLonEvery)
			};

			Vector3 screenA = Transform(Transform(a, viewProjectionMatrix), viewportMatrix);
			Vector3 screenB = Transform(Transform(b, viewProjectionMatrix), viewportMatrix);
			Vector3 screenC = Transform(Transform(c, viewProjectionMatrix), viewportMatrix);

		
		}
	}
}

void Calculation::DrawGrid(Matrix4x4& viewProjectionMatrix, Matrix4x4& viewportMatrix) {
	const float  kGridHalfWidth = 2.0f;

	const uint32_t kSubdivision = 10;

	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
		float x = -kGridHalfWidth + kGridEvery * xIndex;

		Vector3 start = { x, 0.0f, -kGridHalfWidth };

		Vector3 end = { x, 0.0f, kGridHalfWidth };

		Vector3 screenStart = Transform(Transform(start, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEnd = Transform(Transform(end, viewProjectionMatrix), viewportMatrix);


	}

	for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {

		float z = -kGridHalfWidth + kGridEvery * zIndex;

		Vector3 start = { -kGridHalfWidth, 0.0f, z };
		Vector3 end = { kGridHalfWidth, 0.0f, z };

		Vector3 screenStart = Transform(Transform(start, viewProjectionMatrix), viewportMatrix);
		Vector3 screenEnd = Transform(Transform(end, viewProjectionMatrix), viewportMatrix);

	}
}