#include"object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<DirectionalLight> gDirectionLight : register(b1); // b1���W�X�^��C++���ƈ�v������

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // ��{�F��ݒ�
    output.color = gMaterial.color * textureColor;

    // ���C�e�B���O�̌v�Z�ƓK�p
    if (gMaterial.enableLighting != 0){
        // �@���𐳋K�����A���C�g�̕����ƃh�b�g�ς����A���ʂ�0-1�ɃN�����v
        float NdotL = dot(normalize(input.normal), -gDirectionLight.direction);
        float cosFactor = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        // ���C�g�̐F�Ƌ��x��K�p
        float32_t4 lightColor = gDirectionLight.color * gDirectionLight.intensity;

        // �ŏI�I�ȐF�Ƀ��C�e�B���O��K�p
        output.color.rgb *= lightColor.rgb * cosFactor; // RGB�����݂̂ɓK�p
    }

    return output;
}