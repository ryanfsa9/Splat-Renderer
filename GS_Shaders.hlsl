cbuffer ConstantBuffer
{
    float4x4 ProjView;
    float3x3 View;
};

struct VIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
    //float3x3 cov_world : TEXCOORD0;
};

struct VOut
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

struct GOut
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

//Vertex Shader. Takes in points (Gaussians) and converts their positions and cov matrices to screen space
VOut vertexShader(VIn i)
{
    VOut o;
    o.pos = mul(ProjView, float4(i.pos, 1.0f));
    o.col = i.col;
    return o;
}

//Geometry Shader. Constructs oriented rectangles for each Gaussian based on output of vertexShader
[maxvertexcount(3)]
void geometryShader(point VOut i[1], inout TriangleStream<GOut> o)
{
    GOut g1, g2, g3;
    g1.pos = i[0].pos + float4(-0.2, -0.2, 0.0, 0.0);
    g2.pos = i[0].pos + float4(0.0, 0.0, 0.0, 0.0);
    g3.pos = i[0].pos + float4(0.2, -0.2, 0.0, 0.0);
    
    g1.col = i[0].col;
    g2.col = i[0].col;
    g3.col = i[0].col;
    
    o.Append(g1);
    o.Append(g2);
    o.Append(g3);
}

//Pixel Shader. Determines final color and alpha
float4 pixelShader(GOut i) : SV_TARGET
{
    return i.col;
}