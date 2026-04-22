#pragma once
#include "Model.h"
#include "Calculation.h"
#include "Object3dCommon.h"
#include "Structs.h"
#include "Camera.h"
#include <wrl.h>

class Object3d {
public:
    void Initialize(Object3dCommon* common);
    void Update();
    void Draw();
    void ImGui(); // 追加

    // Setter
    void SetModel(Model* model) { this->model = model; }
    void SetScale(const Vector3& scale) { transform.scale = scale; }
    void SetRotate(const Vector3& rotate) { transform.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform.translate = translate; }
    void SetCamera(Camera* camera) { this->camera = camera; }
    void SetColor(const Vector4& color) { materialData->color = color; }

private:
    Object3dCommon* common = nullptr;
    Model* model = nullptr;
    Calculation calculation;

    // 行列計算用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
    TransformationMatrix* wvpData = nullptr;

    // マテリアル用のリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    Material* materialData = nullptr;

    // 座標情報
    Transform transform;
    
    Camera* camera = nullptr;
};

