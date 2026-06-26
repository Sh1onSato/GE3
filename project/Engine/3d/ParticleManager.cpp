#include "ParticleManager.h"
#include "CameraManager.h"
#include "Framework.h"
#include <random>

void ParticleManager::Initialize(ParticleCommon* common, uint32_t textureIndex, ParticleDrawType drawType) {
    this->common = common;
    this->textureIndex = textureIndex;
    this->drawType = drawType;

    // 1. インスタンシング用リソースの作成 (StructuredBuffer)
    instancingResource = common->GetDxCommon()->CreatBufferResource(sizeof(ParticleForGPU) * kMaxParticles);
    instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));

    // 2. 頂点リソースの作成 (最大4頂点の領域を確保)
    vertexResource = common->GetDxCommon()->CreatBufferResource(sizeof(VertexData) * 4);
    VertexData* vertexData = nullptr;
    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    uint32_t vertexCount = 1;
    if (drawType == ParticleDrawType::kPoint) {
        vertexData[0] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 0.5f}, {0.0f, 0.0f, -1.0f} };
        vertexCount = 1;
    } else if (drawType == ParticleDrawType::kLine) {
        // 線（とりあえずY軸方向に少し伸びる線）
        vertexData[0] = { {0.0f, -0.5f, 0.0f, 1.0f}, {0.5f, 0.0f}, {0.0f, 0.0f, -1.0f} };
        vertexData[1] = { {0.0f,  0.5f, 0.0f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f, -1.0f} };
        vertexCount = 2;
    } else {
        // 四角形（面）
        vertexData[0] = { {-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 左下
        vertexData[1] = { {-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 左上
        vertexData[2] = { { 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f} }; // 右下
        vertexData[3] = { { 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} }; // 右上
        vertexCount = 4;
    }

    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * vertexCount;
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

    // 線タイプの場合、速度の大きさに応じてスケール（線の長さ）を引き伸ばす
    // 速さが速いほど長い火花になり、よりリアルな演出になる
    if (drawType == ParticleDrawType::kLine) {
        float speed = Calculation::Length(velocity);
        float lengthScale = speed * 10.0f; // 10倍してちょうど良い長さに調整
        p.transform.scale = { lengthScale, lengthScale, lengthScale };
    }

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

        // ワールド行列の作成
        Matrix4x4 worldMatrix;
        if (drawType == ParticleDrawType::kLine) {
            // 線タイプ: 速度ベクトル方向に線を向ける
            // 頂点バッファはY軸方向（{0,-0.5,0}～{0,+0.5,0}）に線を定義しているため、
            // Y+方向を速度ベクトル方向へ向けるクォータニオンを計算する
            float speed = Calculation::Length(it->velocity);
            Matrix4x4 rotMatrix;
            if (speed > 0.0001f) {
                // デフォルトのY+軸ベクトル
                Vector3 defaultUp = { 0.0f, 1.0f, 0.0f };
                // 正規化した速度ベクトルを目標方向とする
                Vector3 targetDir = Calculation::Normalize(it->velocity);
                // 2つのベクトルの外積から回転軸を求める
                // 外積 = defaultUp x targetDir
                Vector3 axis = {
                    defaultUp.y * targetDir.z - defaultUp.z * targetDir.y,
                    defaultUp.z * targetDir.x - defaultUp.x * targetDir.z,
                    defaultUp.x * targetDir.y - defaultUp.y * targetDir.x
                };
                float axisLen = Calculation::Length(axis);
                if (axisLen > 0.0001f) {
                    // 回転軸を正規化し、回転角度（内積からcos）を求める
                    axis = Calculation::Normalize(axis);
                    float cosAngle = Calculation::Dot(defaultUp, targetDir);
                    // clampしてacosの安全な入力範囲に収める（Windowsマクロとの競合を避けるため括弧でくくる）
                    cosAngle = (std::max)(-1.0f, (std::min)(1.0f, cosAngle));
                    float angle = std::acos(cosAngle);
                    Quaternion q = Calculation::MakeAxisAngleQuaternion(axis, angle);
                    rotMatrix = Calculation::MakeRotateMatrix(q);
                } else {
                    // defaultUpとtargetDirがほぼ同じ（または正反対）の場合
                    if (Calculation::Dot(defaultUp, targetDir) < 0.0f) {
                        // 正反対（速度がY-方向）: Z軸で180度回転
                        Quaternion q = Calculation::MakeAxisAngleQuaternion({ 0.0f, 0.0f, 1.0f }, 3.14159265f);
                        rotMatrix = Calculation::MakeRotateMatrix(q);
                    } else {
                        // 同じ方向: 単位行列
                        rotMatrix = Calculation::MakeIdentity4x4();
                    }
                }
            } else {
                rotMatrix = Calculation::MakeIdentity4x4();
            }
            // スケール → 回転 → 平行移動 の順に合成
            Matrix4x4 scaleMatrix = Calculation::MakeScaleMatrix(it->transform.scale);
            Matrix4x4 translateMatrix = Calculation::MakeTranslationMatrix(it->transform.translate);
            worldMatrix = scaleMatrix * rotMatrix * translateMatrix;
        } else {
            // 点・面タイプ: 従来通りビルボード行列を使用
            worldMatrix = Calculation::MakeBillboardMatrix(it->transform.scale, it->transform.translate, viewMatrix);
        }
        
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

    // 描画タイプに応じた頂点数でインスタンシング描画
    uint32_t vertexCount = 1;
    if (drawType == ParticleDrawType::kLine) {
        vertexCount = 2;
    } else if (drawType == ParticleDrawType::kTriangle) {
        vertexCount = 4;
    }
    commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);
}   commandList->DrawInstanced(vertexCount, instanceCount, 0, 0);
}