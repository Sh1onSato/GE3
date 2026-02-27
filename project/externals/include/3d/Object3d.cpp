#include "Object3d.h"
void Object3d::Initialize(Object3dCommon* common) {
    this->common = common;

    // WVP用のリソース作成
    wvpResource = common->GetDxCommon()->CreatBufferResource(sizeof(TransformationMatrix));
    wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

    // 単位行列で初期化
    wvpData->World = calculation.MakeIdentity4x4();
    wvpData->WVP = calculation.MakeIdentity4x4();

    // デフォルトのトランスフォーム
    transform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
}

void Object3d::Update() {
    // 1. ワールド行列の計算
    Matrix4x4 worldMatrix = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // 2. ビュー・プロジェクション行列の作成
    Matrix4x4 cameraMatrix = calculation.MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
    Matrix4x4 viewMatrix = calculation.Inverse(cameraMatrix);
    Matrix4x4 projectionMatrix = calculation.MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

    Matrix4x4 wvpMatrix = calculation.Multiply(calculation.Multiply(projectionMatrix, viewMatrix), worldMatrix);
    // 3. データ転送
    wvpData->World = calculation.Transpose(worldMatrix);;
    wvpData->WVP = calculation.Transpose(wvpMatrix);
}

void Object3d::Draw() {
    if (!model) { return; } // モデルがセットされていなければ何もしない

    auto commandList = common->GetDxCommon()->GetCommandList();

    // WVPバッファをセット（b1に割り当てている想定）
    commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

    // モデル自身の描画処理（頂点バッファセットなど）を呼ぶ
    model->Draw(commandList);
}