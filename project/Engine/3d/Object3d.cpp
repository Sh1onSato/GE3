#include "Object3d.h"
#include "externals/imgui/imgui.h"
#include "CameraManager.h"

void Object3d::Initialize(Object3dCommon* common) {
    this->common = common;

    // WVP用のリソース作成
    wvpResource = common->GetDxCommon()->CreatBufferResource(sizeof(TransformationMatrix));
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

    // マテリアル用のリソース作成
    materialResource = common->GetDxCommon()->CreatBufferResource(sizeof(Material));
    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // 初期化
    wvpData->World = Calculation::MakeIdentity4x4();
    wvpData->WVP = Calculation::MakeIdentity4x4();

    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData->enableLighting = 1;
    materialData->uvTransform = Calculation::MakeIdentity4x4();

    // デフォルトのトランスフォーム
    transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    quaternionRotation = Calculation::IdentityQuaternion();
}

void Object3d::SetRotate(const Vector3& rotate) {
    transform.rotate = rotate;
    // オイラー角からクォータニオンを作成
    Quaternion qX = Calculation::MakeAxisAngleQuaternion({ 1.0f, 0.0f, 0.0f }, rotate.x);
    Quaternion qY = Calculation::MakeAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, rotate.y);
    Quaternion qZ = Calculation::MakeAxisAngleQuaternion({ 0.0f, 0.0f, 1.0f }, rotate.z);
    quaternionRotation = Calculation::Multiply(qX, Calculation::Multiply(qY, qZ));
}

void Object3d::Update() {
    // 1. ワールド行列の計算 (クォータニオンを使用)
    Matrix4x4 worldMatrix = Calculation::MakeAffineMatrix(transform.scale, quaternionRotation, transform.translate);

    // 2. ビュー・プロジェクション行列の取得
    Matrix4x4 viewProjectionMatrix;
    Camera* currentCamera = camera;
    if (!currentCamera) {
        currentCamera = CameraManager::GetInstance()->GetActiveCamera();
    }

    if (currentCamera) {
        viewProjectionMatrix = currentCamera->GetViewProjectionMatrix();
    } else {
        viewProjectionMatrix = Calculation::MakeIdentity4x4();
    }

    Matrix4x4 wvpMatrix = worldMatrix * viewProjectionMatrix;
    // 3. データ転送
    wvpData->World = worldMatrix;
    wvpData->WVP = wvpMatrix;
}

void Object3d::Draw() {
    if (!model) { return; } // モデルがセットされていなければ何もしない

    auto commandList = common->GetDxCommon()->GetCommandList();

    // マテリアルバッファをセット (b0)
    commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    // WVPバッファをセット (b1)
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

    // モデル自身の描画処理
    model->Draw(commandList);
}

void Object3d::ImGui() {
    if (ImGui::BeginTabBar("Object3dTabBar")) {
        if (ImGui::BeginTabItem("Transform")) {
            ImGui::DragFloat3("Position", &transform.translate.x, 0.1f);
            if (ImGui::DragFloat3("Rotation (Euler)", &transform.rotate.x, 0.01f)) {
                SetRotate(transform.rotate); // オイラー角を更新したらクォータニオンも更新
            }
            ImGui::DragFloat4("Quaternion", &quaternionRotation.x, 0.01f);
            if (ImGui::Button("Normalize Quaternion")) {
                quaternionRotation = Calculation::Normalize(quaternionRotation);
            }
            ImGui::DragFloat3("Scale", &transform.scale.x, 0.1f);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Material")) {
            ImGui::ColorEdit4("Color", &materialData->color.x);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Camera")) {
            if (camera) {
                ImGui::DragFloat3("Camera Position", &camera->GetTransform().translate.x, 0.1f);
                ImGui::DragFloat3("Camera Rotation", &camera->GetTransform().rotate.x, 0.01f);
            } else {
                ImGui::Text("Camera is not set.");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}