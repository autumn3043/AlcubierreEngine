struct VSOutput
{
    float4 pos : SV_Position;
    float3 normal : TEXCOORD1;
    float2 uv : TEXCOORD0;
};

Texture2D diffuseTexture : register(t0, space1);
SamplerState diffuseSampler : register(s0, space1);
Texture2D bumpTexture : register(t1, space1);
SamplerState bumpSampler : register(s1, space1);
Texture2D displacementTexture : register(t2, space1);
SamplerState displacementSampler : register(s2, space1);
Texture2D specularTexture : register(t3, space1);
SamplerState specularSampler : register(s3, space1);
Texture2D specularHighlightTexture : register(t4, space1);
SamplerState specularHighlightSampler : register(s4, space1);
Texture2D alphaTexture : register(t5, space1);
SamplerState alphaSampler : register(s5, space1);
Texture2D reflectionTexture : register(t6, space1);
SamplerState reflectionSampler : register(s6, space1);

[shader("fragment")]
float4 fragment(VSOutput vertIn) : SV_TARGET {
    float4 colour = diffuseTexture.Sample(diffuseSampler, vertIn.uv);
    return colour;
}