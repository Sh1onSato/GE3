#include "ParticleEmitter.h"
#include <random>

void ParticleEmitter::Initialize(const std::string& name, ParticleManager* manager, const EmitterSetting& setting) {
    this->name = name;
    this->manager = manager;
    this->setting = setting;
    this->timer = 0.0f;

    // 乱数生成器の初期化
    std::random_device seed_gen;
    randomEngine.seed(seed_gen());
}

void ParticleEmitter::Update() {
    timer += 1.0f / 60.0f; // 暫定 60FPS

    if (timer >= setting.frequency) {
        timer -= setting.frequency;

        // 指定された数だけ放出
        for (uint32_t i = 0; i < setting.count; ++i) {
            Vector3 velocity;

            // X軸
            if (setting.minVelocity.x == setting.maxVelocity.x) {
                velocity.x = setting.minVelocity.x;
            } else {
                float minV = (std::min)(setting.minVelocity.x, setting.maxVelocity.x);
                float maxV = (std::max)(setting.minVelocity.x, setting.maxVelocity.x);
                std::uniform_real_distribution<float> dist(minV, maxV);
                velocity.x = dist(randomEngine);
            }

            // Y軸
            if (setting.minVelocity.y == setting.maxVelocity.y) {
                velocity.y = setting.minVelocity.y;
            } else {
                float minV = (std::min)(setting.minVelocity.y, setting.maxVelocity.y);
                float maxV = (std::max)(setting.minVelocity.y, setting.maxVelocity.y);
                std::uniform_real_distribution<float> dist(minV, maxV);
                velocity.y = dist(randomEngine);
            }

            // Z軸
            if (setting.minVelocity.z == setting.maxVelocity.z) {
                velocity.z = setting.minVelocity.z;
            } else {
                float minV = (std::min)(setting.minVelocity.z, setting.maxVelocity.z);
                float maxV = (std::max)(setting.minVelocity.z, setting.maxVelocity.z);
                std::uniform_real_distribution<float> dist(minV, maxV);
                velocity.z = dist(randomEngine);
            }

            manager->Emit(setting.position, velocity, setting.color, setting.lifeTime);
        }
    }
}
