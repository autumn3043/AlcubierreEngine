struct VSInput {
    float3 pos;
    float3 normal;
    float2 uv;
};

struct Global {
    float4x4 view;
    float4x4 proj;
}
ConstantBuffer<Global> global;
struct Object {
    float4x4 model;
}
ConstantBuffer<Object> object;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 normal: TEXCOORD1;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VSOutput vertex(VSInput input) {
    VSOutput output;
    output.pos = mul(global.proj, mul(global.view, mul(object.model, float4(input.pos, 1.0))));
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}