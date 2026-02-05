#include "Sprite.h"
#include "SpriteCommon.h"
#include"calculation.h"
#include"externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include"externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



void Sprite::Initialize(SpriteCommon* spriteCommon,uint32_t textureIndex){
    this->spriteCommon = spriteCommon;
    this->dxCommon = spriteCommon->GetDxCommon();
    // Commonに「住所を教えて！」と頼んで、自分の中にメモ（変数）しておく
    this->textureSrvHandleGPU = spriteCommon->GetTextureHandle(textureIndex);

    // 1. 頂点リソースの設定 (4頂点分でOK)
    vertexResourceSprite = dxCommon->CreatBufferResource(sizeof(VertexData) * 4);
    // .hのメンバ変数 vertexBufferViewSprite に代入
    vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
    vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

    // 頂点データの書き込み
    vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&VertexDataSprite));
    VertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; VertexDataSprite[0].texcoord = { 0.0f, 1.0f };
    VertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };   VertexDataSprite[1].texcoord = { 0.0f, 0.0f };
    VertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; VertexDataSprite[2].texcoord = { 1.0f, 1.0f };
    VertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };   VertexDataSprite[3].texcoord = { 1.0f, 0.0f };

    // 2. インデックスリソースの設定 (6要素)
    indexResourceSprite = dxCommon->CreatBufferResource(sizeof(uint32_t) * 6);
    indexBuffViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
    indexBuffViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
    indexBuffViewSprite.Format = DXGI_FORMAT_R32_UINT;

    indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
    indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
    indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;

    // 3. マテリアルリソースの設定
    materialResourceSprite = dxCommon->CreatBufferResource(sizeof(Material));
    materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData->enableLighting = false;
    materialData->uvTransform = calculation.MakeIdentity4x4();

    // 4. 行列リソースの設定
    transformationMatrixResourceSprite = dxCommon->CreatBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformetionMatrixDataSprite));
    transformetionMatrixDataSprite->World = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
    transformetionMatrixDataSprite->WVP = calculation.MakeIdentity4x4();
}

void Sprite::Update() {
    // 1. スプライトのワールド行列を計算
    Matrix4x4 worldMatrix = calculation.MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

    // 2. ビュー行列（スプライトは通常単位行列）
    Matrix4x4 viewMatrix = calculation.MakeIdentity4x4();

    // 3. プロジェクション行列（平行投影 / 正射影行列）
    // 本来は WinApp の定数や SpriteCommon からサイズをもらうのが良い
    Matrix4x4 projectionMatrix = calculation.MakeOrthographicMatrix(
        0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);

    // 4. WVP行列の合成
    Matrix4x4 wvpMatrix = calculation.Multiply(worldMatrix, calculation.Multiply(viewMatrix, projectionMatrix));

    // 5. GPU上のリソース（定数バッファ）に書き込む
    // Initialize で Map したポインタを保持している場合は、そこに直接代入するだけでOK
    transformetionMatrixDataSprite->WVP = wvpMatrix;
    transformetionMatrixDataSprite->World = worldMatrix;


    Matrix4x4 uvTransformMatrix = calculation.MakeScaleMatrix(uvTransformSprite.scale);
    uvTransformMatrix = calculation.Multiply(uvTransformMatrix, calculation.MakeRotationZMatrix(uvTransformSprite.rotate.z));
    uvTransformMatrix = calculation.Multiply(uvTransformMatrix, calculation.MakeTranslationMatrix(uvTransformSprite.translate));
    materialData->uvTransform = uvTransformMatrix; // UV変換行列を更新

}

void Sprite::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

    // 1. 頂点とインデックスをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
    commandList->IASetIndexBuffer(&indexBuffViewSprite);

    // 2. 定数バッファをセット (RootParameterの番号に合わせる)
    commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

    // 3. テクスチャをセット (今のところ main からもらったハンドルを使う想定)
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

    // 4. 描画！
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void Sprite::ImGui(){
    ImGui::Begin("Sprite Debug"); // ウィンドウの名前（任意）

    // UV変換の操作
    ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
    ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
    ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

    ImGui::End();
}
