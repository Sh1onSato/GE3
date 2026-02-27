#pragma once
#include "Model.h"
#include "Calculation.h"
#include "Object3dCommon.h"
#include "Structs.h"
#include <wrl.h>

class Object3d {
public:
    void Initialize(Object3dCommon* common);
    void Update();
    void Draw();

    // Setter
    void SetModel(Model* model) { this->model = model; }
    void SetScale(const Vector3& scale) { transform.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform.translate = translate; }
    void SetCameraTransform(const Transform& cameraTransform) { this->cameraTransform = cameraTransform; }

private:
    Object3dCommon* common = nullptr;
    Model* model = nullptr;
    Calculation calculation;

    // 行列計算用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
    TransformationMatrix* wvpData = nullptr;
    // 座標情報
    Transform transform;
    // 本来はCameraクラスから持ってくるべきだが、一旦ここで保持
    Transform cameraTransform;
};

