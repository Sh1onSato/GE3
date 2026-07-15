struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gTexture : register(t0);
Texture2D<float4> gBloomTexture : register(t1);
Texture2D<float4> gBlurTexture : register(t2);
SamplerState gSampler : register(s0);

// ブルームをどれだけ加算合成するかの強度／グレースケール化・セピア化・画面全体ぼかしをするかどうか
cbuffer CompositeParam : register(b0) {
    float gBloomIntensity;
    float gGrayscale;   // 0=通常表示, 1=グレースケール表示
    float gSepia;       // 0=通常表示, 1=セピア表示（gGrayscaleより優先）
    float gBlurEnabled; // 0=通常表示, 1=画面全体ぼかし表示（ブルームとは独立して重ねがけされる）
};

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;

    float4 sceneColor = gTexture.Sample(gSampler, input.texcoord);

    if (gBlurEnabled > 0.5f) {
        sceneColor = gBlurTexture.Sample(gSampler, input.texcoord);
    }

    float4 bloomColor = gBloomTexture.Sample(gSampler, input.texcoord);

    output.color.rgb = sceneColor.rgb + bloomColor.rgb * gBloomIntensity;

    if (gSepia > 0.5f) {
        // 回想シーン風のセピア調（定番の変換行列で赤み・黄み寄りの色に写像する）
        float3 sepiaColor;
        sepiaColor.r = dot(output.color.rgb, float3(0.393f, 0.769f, 0.189f));
        sepiaColor.g = dot(output.color.rgb, float3(0.349f, 0.686f, 0.168f));
        sepiaColor.b = dot(output.color.rgb, float3(0.272f, 0.534f, 0.131f));
        output.color.rgb = saturate(sepiaColor);
    } else {
        // 輝度に変換し、gGrayscaleの値に応じて通常色とブレンドする（1.0で完全グレースケール）
        float luminance = dot(output.color.rgb, float3(0.299f, 0.587f, 0.114f));
        output.color.rgb = lerp(output.color.rgb, luminance.xxx, gGrayscale);
    }

    output.color.a = sceneColor.a;

    return output;
}
