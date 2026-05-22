#include "ParticleManager.h"
#include "CameraManager.h"
#include "Framework.h"
#include <random>

void ParticleManager::Initialize(ParticleCommon* common, uint32_t textureIndex) {
    this->common = common;
    this->textureIndex = textureIndex;

    // 1. インスタンシング用リソースの作成 (StructuredBuffer)
    instancingResource = common->GetDxCommon()->CreatBufferResource(sizeof(ParticleForGPU) * kMaxParticles);
    instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));

    // 2. 頂点リソースの作成 (四角形板ポリゴン)
    vertexResource = common->GetDxCommon()->CreatBufferResource(sizeof(VertexData) * 4);
    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    // 左下
    vertexData[0] = { {-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} };
    // 左上
    vertexData[1] = { {-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} };
    // 右下
    vertexData[2] = { { 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} };
    // 右上
    vertexData[3] = { { 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} };

    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferView.StrideInBytes = sizeof(VertexData);
}

void ParticleManager::Emit(const Vector3& position, const Vector3& velocity, const Vector4& color, float lifeTime) {
    // 最大数を超えていたら追加しない
    if (particles.size() >= kMaxParticles) {
        return;
    }

    Particle p;
    p.transform.translate = position;
    p.transform.rotate = { 0, 0, 0 };
    p.transform.scale = { 1, 1, 1 };
    p.velocity = velocity;
    p.color = color;
    p.lifeTime = lifeTime;
    p.currentTime = 0;

    particles.push_back(p);
}

void ParticleManager::Update() {
    Camera* camera = CameraManager::GetInstance()->GetActiveCamera();
    Matrix4x4 viewProjectionMatrix = camera ? camera->GetViewProjectionMatrix() : Calculation::MakeIdentity4x4();
    Matrix4x4 viewMatrix = camera ? camera->GetViewMatrix() : Calculation::MakeIdentity4x4();

    instanceCount = 0;
    for (auto it = particles.begin(); it != particles.end(); ) {
        // 寿命チェック
        it->currentTime += 1.0f / 60.0f; // 暫定 60FPS
        if (it->currentTime >= it->lifeTime) {
            it = particles.erase(it);
            continue;
        }

        // 移動
        it->transform.translate.x += it->velocity.x;
        it->transform.translate.y += it->velocity.y;
        it->transform.translate.z += it->velocity.z;

        // ビルボード行列の作成 (回転をカメラに合わせる)
        Matrix4x4 worldMatrix = Calculation::MakeBillboardMatrix(it->transform.scale, it->transform.translate, viewMatrix);
        
        // GPU用データに書き込み
        if (instanceCount < kMaxParticles) {
            instancingData[instanceCount].World = worldMatrix;
            instancingData[instanceCount].WVP = worldMatrix * viewProjectionMatrix;
            
            float alpha = 1.0f - (it->currentTime / it->lifeTime);
            instancingData[instanceCount].color = it->color;
            instancingData[instanceCount].color.w *= alpha;
            instanceCount++;
        }

        ++it;
    }
}

void ParticleManager::Draw() {
    if (instanceCount == 0) return;

    auto commandList = common->GetDxCommon()->GetCommandList();

    // VBVをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // StructuredBuffer (t0) をセット
    commandList->SetGraphicsRootShaderResourceView(0, instancingResource->GetGPUVirtualAddress());

    // テクスチャ (t1) をセット (Framework経由でSrvManagerを取得)
    SrvManager* srvManager = Framework::GetInstance()->GetSrvManager();
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = srvManager->GetGPUDescriptorHandle(textureIndex);
    commandList->SetGraphicsRootDescriptorTable(1, textureHandle);

    // インスタンシング描画 (実際に更新された数だけ描画)
    commandList->DrawInstanced(4, instanceCount, 0, 0);
}
