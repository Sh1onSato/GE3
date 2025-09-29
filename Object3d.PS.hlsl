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

ConstantBuffer<DirectionalLight> gDirectionLight : register(b1); // b1レジスタはC++側と一致させる

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 基本色を設定
    output.color = gMaterial.color * textureColor;

    // ライティングの計算と適用
    if (gMaterial.enableLighting != 0){
        // 法線を正規化し、ライトの方向とドット積を取り、結果を0-1にクランプ
        float NdotL = dot(normalize(input.normal), -gDirectionLight.direction);
        float cosFactor = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        // ライトの色と強度を適用
        float32_t4 lightColor = gDirectionLight.color * gDirectionLight.intensity;

        // 最終的な色にライティングを適用
        output.color.rgb *= lightColor.rgb * cosFactor; // RGB成分のみに適用
    }

    return output;
}