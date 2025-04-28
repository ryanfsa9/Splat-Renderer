cbuffer ConstantBuffer
{
    float4x4 ProjView;
    float3x3 View;
};

struct VIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float3x3 cov : MATRIX;
};

struct VOut
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float3 scales : TEXCOORD0;
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
    o.scales = float3(i.cov[0][0], i.cov[1][1], i.cov[2][2]);
    return o;
}

//Geometry Shader. Constructs oriented rectangles for each Gaussian based on output of vertexShader
[maxvertexcount(3)]
void geometryShader(point VOut i_[1], inout TriangleStream<GOut> o)
{
    VOut i = i_[0];
    GOut g1, g2, g3;
    g1.pos = i.pos + float4(-0.01, -0.01, 0.0, 0.0);
    g2.pos = i.pos + float4(0.0, 0.0, 0.0, 0.0);
    g3.pos = i.pos + float4(0.01, -0.01, 0.0, 0.0);
    
    g1.col = i.col;
    g2.col = i.col;
    g3.col = i.col;
    
    o.Append(g1);
    o.Append(g2);
    o.Append(g3);
}

//Pixel Shader. Determines final color and alpha
float4 pixelShader(GOut i) : SV_TARGET
{
    return i.col;
}