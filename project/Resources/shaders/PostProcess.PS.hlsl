struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // オフスクリーンで描画したシーン全体の色をサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    output.color = textureColor;
    
    // --- 【お試し】モノクロ化したい場合はここを有効にする ---
    // float gray = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    // output.color.rgb = float3(gray, gray, gray);
    
    return output;
}
