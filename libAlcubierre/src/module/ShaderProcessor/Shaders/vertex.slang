struct VSInput {
    float3 inPosition;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
}
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    float4 pos : SV_Position;
};

[shader("vertex")]
VSOutput vertex(VSInput input) {
    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
    return output;
}