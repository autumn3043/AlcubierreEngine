struct VSOutput
{
    float4 pos : SV_Position;
};

[shader("fragment")]
float4 fragment(VSOutput vertIn) : SV_TARGET {
    return float4(1.0, 1.0, 1.0, 1.0);
}