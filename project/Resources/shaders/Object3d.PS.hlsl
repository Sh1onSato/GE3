#include"object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float2 padding;
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
    float padding;
};

struct ShadowData
{
    float4x4 lightViewProjection;
    float bias;
    float3 padding;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<DirectionalLight> gDirectionLight : register(b2); // b1レジスタはC++側と一致させる
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);
ConstantBuffer<ShadowData> gShadowData : register(b5);

Texture2D<float> gShadowMap : register(t1);
SamplerComparisonState gShadowSampler : register(s1);

// ディレクショナルライトのシャドウ係数を計算する（1.0=影なし、0.0=完全に影）
static const float kShadowMapTexelSize = 1.0f / 4096.0f;
// 法線方向のオフセット量（壁のように光に対してほぼ真横を向く面でのシャドウアクネ対策）
static const float kNormalOffsetScale = 0.05f;
float CalculateDirectionalShadow(float3 worldPosition, float3 normal, float NdotL)
{
    // 法線方向に少しずらしてからライト空間へ変換することで、グレージング角での自己遮蔽誤判定を抑える
    float3 offsetWorldPosition = worldPosition + normal * kNormalOffsetScale;
    float4 lightClipPos = mul(float4(offsetWorldPosition, 1.0f), gShadowData.lightViewProjection);
    float3 ndc = lightClipPos.xyz / lightClipPos.w;

    float2 shadowUV = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);

    // ライトの正射影範囲外は影なし扱い（誤って影が落ちるのを防ぐ）
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f) {
        return 1.0f;
    }

    // 光に対して面が斜めになるほどバイアスを強める（スロープスケールバイアス）
    float slopeScaledBias = gShadowData.bias * max(1.0f, (1.0f - saturate(NdotL)) * 4.0f);

    float shadowFactor = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            float2 offset = float2(x, y) * kShadowMapTexelSize;
            shadowFactor += gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, ndc.z - slopeScaledBias);
        }
    }
    shadowFactor /= 9.0f;

    return shadowFactor;
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 基本色を設定
    output.color = gMaterial.color * textureColor;

    // アルファ値が0.5以下ならピクセルを破棄（2値抜き）
    if (output.color.a <= 0.5f) {
        discard;
    }

    // ライティングの計算と適用
    if (gMaterial.enableLighting != 0){
        float3 baseColor = output.color.rgb;
        float3 normal = normalize(input.normal);
        float3 lightingResult = float3(0.0f, 0.0f, 0.0f);

        // ディレクショナルライト（シャドウマップ + PCFで影を落とす）
        {
            float NdotL = dot(normal, -gDirectionLight.direction);
            float cosFactor = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float shadowFactor = CalculateDirectionalShadow(input.worldPosition, normal, NdotL);
            lightingResult += gDirectionLight.color.rgb * gDirectionLight.intensity * cosFactor * shadowFactor;
        }

        // 点光源
        {
            float3 toLight = gPointLight.position - input.worldPosition;
            float distance = length(toLight);
            float3 lightDirection = toLight / distance;
            float NdotL = dot(normal, lightDirection);
            float cosFactor = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float attenuation = pow(saturate(-distance / gPointLight.radius + 1.0f), gPointLight.decay);
            lightingResult += gPointLight.color.rgb * gPointLight.intensity * cosFactor * attenuation;
        }

        // スポットライト
        {
            float3 toLight = gSpotLight.position - input.worldPosition;
            float distance = length(toLight);
            float3 lightDirection = toLight / distance;
            float NdotL = dot(normal, lightDirection);
            float cosFactor = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float attenuation = pow(saturate(-distance / gSpotLight.distance + 1.0f), gSpotLight.decay);
            float cosAngle = dot(-lightDirection, gSpotLight.direction);
            float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
            lightingResult += gSpotLight.color.rgb * gSpotLight.intensity * cosFactor * attenuation * falloffFactor;
        }

        // 環境光（どの光源も直接当たらない面が完全な黒潰れにならないよう底上げする）
        static const float kAmbientIntensity = 0.08f;
        float3 ambient = baseColor * kAmbientIntensity;

        output.color.rgb = baseColor * lightingResult + ambient;
    }

    return output;
}