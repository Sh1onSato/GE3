#include "Particle.hlsli"

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) {
    VertexShaderOutput output;
    output.position = mul(input.position, gParticle[instanceId].WVP);
    output.texcoord = input.texcoord * gParticle[instanceId].uvScale + gParticle[instanceId].uvOffset;
    output.normal = mul(input.normal, (float3x3)gParticle[instanceId].World);
    output.color = gParticle[instanceId].color;
    return output;
}
