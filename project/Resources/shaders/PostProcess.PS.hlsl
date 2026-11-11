// PostProcess の最後の段: 3D の絵にブルームを足し、色を整え、ヴィネットを掛けて画面（黒帯の内側）に写す
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gScene : register(t0); // 3D の絵（画面全体のぼかしを掛けたときはぼかした絵）
Texture2D<float4> gBloom : register(t1); // ぼかした明るい所
Texture2D<float4> gAmbientOcclusion : register(t2); // 接している所の暗さ（SSAO。1 で暗くしない）
SamplerState gLinearSampler : register(s0);
SamplerState gPointSampler : register(s1);

// PostProcess::Render の constants と同じ並び
cbuffer CompositeParams : register(b0) {
    float gBloomIntensity;
    float gBrightness;
    float gContrast;
    float gSaturation;
    float gSepia;
    float gVignetteIntensity;
    float gVignetteRadius;
    float gUsePointFilter; // 1 なら引き伸ばしをくっきり（RenderScaler の kPoint）
    float3 gTint;
    float gUseAmbientOcclusion; // 1 なら gAmbientOcclusion を掛ける
    float3 gVignetteColor;
    float gAspect; // 絵の幅 / 高さ（ヴィネットを楕円でなく円にする）
    // シーン遷移のグリッチ: x = 強さ（0 で無し、1 で最大）、y = 乱数の種にする経過時間[秒]、z = 画面の高さ[ピクセル]（走査線の本数用）、w = 予備
    float4 gGlitch;
};

// --- グリッチ演出の調整値 ---
static const float kGlitchBandCount = 24.0f;       // 画面を縦に分割する帯の数
static const float kGlitchRate = 20.0f;            // 乱数が切り替わる速さ(回/秒)
static const float kGlitchShiftMax = 0.05f;        // ブロックずれの最大量(UV)。強度を掛けて使う
static const float kGlitchRgbOffset = 0.006f;      // RGBずれの量(UV)。強度を掛けて使う
static const float kGlitchScanlineDarken = 0.15f;  // 走査線で下げる明度の最大量。強度を掛けて使う
static const float kGlitchDefaultScreenHeight = 720.0f; // gGlitch.z が 0 のときの画面の高さ
static const float kGlitchPi = 3.14159265f;

// 2次元の値から0～1の疑似乱数を作る小さなハッシュ
float GlitchHash(float2 p) {
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float4 main(VertexShaderOutput input) : SV_TARGET {
    // シーン遷移のグリッチ（強さが 0 より大きいときだけ）: 帯ごとの乱数で、強さに比例した確率の帯だけ横にずらす
    float2 sceneUV = input.texcoord;
    float rgbOffset = 0.0f;
    const bool useGlitch = gGlitch.x > 0.0f;
    if (useGlitch) {
        float band = floor(input.texcoord.y * kGlitchBandCount);
        float tick = floor(gGlitch.y * kGlitchRate);
        float chance = GlitchHash(float2(band, tick));
        if (chance < gGlitch.x) {
            float dir = GlitchHash(float2(band + 17.0f, tick)) * 2.0f - 1.0f; // -1～1
            sceneUV.x += dir * kGlitchShiftMax * gGlitch.x;
        }
        rgbOffset = kGlitchRgbOffset * gGlitch.x;
    }

    float3 color = (gUsePointFilter > 0.5f)
        ? gScene.Sample(gPointSampler, sceneUV).rgb
        : gScene.Sample(gLinearSampler, sceneUV).rgb;

    if (useGlitch) {
        // RGBずれ：Rを+x、Bを-xへずらして別サンプルし、Gはそのまま
        float2 shift = float2(rgbOffset, 0.0f);
        if (gUsePointFilter > 0.5f) {
            color.r = gScene.Sample(gPointSampler, sceneUV + shift).r;
            color.b = gScene.Sample(gPointSampler, sceneUV - shift).b;
        } else {
            color.r = gScene.Sample(gLinearSampler, sceneUV + shift).r;
            color.b = gScene.Sample(gLinearSampler, sceneUV - shift).b;
        }
        // 走査線：細い横縞で明度を下げる
        float screenHeight = (gGlitch.z > 0.0f) ? gGlitch.z : kGlitchDefaultScreenHeight;
        float scan = sin(input.texcoord.y * screenHeight * kGlitchPi) * 0.5f + 0.5f;
        color *= 1.0f - scan * kGlitchScanlineDarken * gGlitch.x;
    }

    // 接している所・すき間を暗くする（光を足す前に掛ける）
    if (gUseAmbientOcclusion > 0.5f) {
        color *= gAmbientOcclusion.Sample(gLinearSampler, sceneUV).r;
    }

    // ブルーム（光を足す。グリッチのブロックずれと同じ UV で引く。強さ 0 では sceneUV == texcoord）
    color += gBloom.Sample(gLinearSampler, sceneUV).rgb * gBloomIntensity;

    // 色の調整: 明るさ → コントラスト（0.5 の灰色を中心に） → 彩度 → セピア → 色を掛ける
    color += gBrightness;
    color = (color - 0.5f) * gContrast + 0.5f;
    float luminance = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    color = lerp(luminance.xxx, color, gSaturation);
    float3 sepiaColor = float3(
        dot(color, float3(0.393f, 0.769f, 0.189f)),
        dot(color, float3(0.349f, 0.686f, 0.168f)),
        dot(color, float3(0.272f, 0.534f, 0.131f)));
    color = lerp(color, sepiaColor, gSepia);
    color *= gTint;

    // ヴィネット: 中心からの距離（0 で中心、1 で角。縦横比を直して円にする）が半径より外側ほど gVignetteColor に寄せる
    if (gVignetteIntensity > 0.0f) {
        float2 centered = (input.texcoord - 0.5f) * float2(gAspect, 1.0f) * 2.0f;
        float distanceFromCenter = length(centered) / sqrt(gAspect * gAspect + 1.0f);
        float amount = smoothstep(gVignetteRadius, 1.0f, distanceFromCenter) * gVignetteIntensity;
        color = lerp(color, gVignetteColor, amount);
    }
    return float4(saturate(color), 1.0f);
}
