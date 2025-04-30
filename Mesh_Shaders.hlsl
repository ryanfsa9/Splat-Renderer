cbuffer ConstantBuffer
{
    row_major float4x4 Proj;
    row_major float4x4 View;
};

struct VIn
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct VOut
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

//the scene lighting is already baked into the mesh colors, so these shaders are as simple as it gets.

VOut vertexShader(VIn i)
{
    VOut o;
    o.pos = mul(Proj, mul(View, float4(i.pos, 1.0f)));
    o.col = i.col;
	return o;
}

float4 pixelShader(VOut i) : SV_TARGET
{
    return float4(i.col, 1.0f);
}