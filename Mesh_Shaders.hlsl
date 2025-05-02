cbuffer ConstantBuffer
{
    row_major float4x4 Proj;
    row_major float4x4 View;
};

struct VIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VOut
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

VOut vertexShader(VIn i)
{
    VOut o;
    o.pos = mul(Proj, mul(View, float4(i.pos, 1.0f)));
    o.normal = i.normal;
	return o;
}

float4 pixelShader(VOut i) : SV_TARGET
{
    float3 sun = normalize(float3(1.0, 1.0, 1.0));
    float l = 0.1f + dot(sun, i.normal);
    l = clamp(l, 0.0, 1.0);
    return float4(l, l, l, 1.0f);
}