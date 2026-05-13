#pragma once
#include "Calculation.h"
#include "Structs.h"

class Camera {
public:
    Camera();
    void Update();
    void ImGui();

    // Getter
    const Transform& GetTransform() const { return transform; }
    Transform& GetTransform() { return transform; }
    const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
    const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }

    // Setter
    void SetRotate(const Vector3& rotate) { transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform.translate = translate; }
    void SetTransform(const Transform& transform) { this->transform = transform; }

private:
    Transform transform;
    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;
    Matrix4x4 viewProjectionMatrix;

    float fovY = 0.45f;
    float aspectRatio = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;
};
