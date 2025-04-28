cbuffer ConstantBuffer
{
    row_major float4x4 Proj;
    row_major float4x4 View;
};

struct VIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float3 cov0 : TEXCOORD0;
    float3 cov1 : TEXCOORD1;
};

struct VOut
{
    float4 pos : POSITION;
    float4 col : COLOR;
    float2 majorAxis : TEXCOORD0;
    float2 minorAxis : TEXCOORD1;
};

struct GOut
{
    float4 pos : SV_POSITION;
    float2 pos_local : POSITION;
    float4 col : COLOR;
};

//Vertex Shader. Takes in points (Gaussians) and converts their positions and cov matrices to screen space
VOut vertexShader(VIn i)
{
    VOut o;
    float4 pos_cam = mul(View, float4(i.pos, 1.0f));
    o.pos = mul(Proj, pos_cam);
    o.col = i.col;
    
    o.pos /= o.pos.w;
    
    //unpack cov
    row_major float3x3 cov =
    {
      i.cov0.x, i.cov0.y, i.cov0.z,
      i.cov0.y, i.cov1.x, i.cov1.y,
      i.cov0.z, i.cov1.y, i.cov1.z
    };
    //project cov onto clip space
    row_major float3x3 J =
    {
        Proj[0][0]/pos_cam.z, 0.0f,                  0.0f,
        0.0f,                 Proj[1][1]/pos_cam.z,  0.0f,
        -Proj[0][0] * pos_cam.x / pos_cam.z / pos_cam.z, Proj[1][1] * pos_cam.y / pos_cam.z / pos_cam.z, 0.0f
    };
    //row_major float3x3 J =
    //{
    //    Proj[0][0] / pos_cam.z, 0.0f, -Proj[0][0] * pos_cam.x / pos_cam.z / pos_cam.z,
    //    0.0f, Proj[1][1] / pos_cam.z, Proj[1][1] * pos_cam.y / pos_cam.z / pos_cam.z,
    //    0.0f, 0.0f, 0.0f
    //};
    
    row_major float3x3 View3 =
    {
        View[0][0], View[0][1], View[0][2],
        View[1][0], View[1][1], View[1][2],
        View[2][0], View[2][1], View[2][2]
    };
    row_major float3x3 JV = mul(J, View3);
    row_major float3x3 cov2d = mul(JV, mul(cov, transpose(JV)));
    //now have 2x2 matrix representing the cov in clip space
    //this matrix represents a rotated ellipse, want to find the ellipses axes (the eigenvectors of cov2d). the eigenvalues are the lengths of the major and minor axes
    
    //find eigenvalues
    float a = (cov2d[0][0] + cov2d[1][1]) / 2.0;
    float b = length(float2((cov2d[0][0] - cov2d[1][1]) / 2.0, cov2d[0][1]));
    float eigen1 = a + b;
    float eigen2 = a - b;
    
    //find an eigenvector of eigen1. this is major axis of the ellipse as eigen1 > eigen2
    float2 eigenVector = normalize(float2(cov2d[0][1], eigen1 - cov2d[0][0]));
        
    //take sqrt to get standard deviation instead of var
    o.majorAxis = sqrt(eigen1) * eigenVector;
    o.minorAxis = sqrt(eigen2) * float2(-eigenVector.y, eigenVector.x);
    
    return o;
}

//Geometry Shader. Constructs oriented rectangles for each Gaussian based on output of vertexShader. spans 2 stddivs in each direction
[maxvertexcount(6)]
void geometryShader(point VOut i_[1], inout TriangleStream<GOut> o)
{
    VOut i = i_[0];
    GOut g1, g2, g3, g4;
    float4 right = float4(i.majorAxis, 0.0f, 0.0f) * 4;
    float4 up = float4(i.minorAxis, 0.0f, 0.0f) * 4;
    g1.pos = i.pos - right - up;
    g2.pos = i.pos - right + up;
    g3.pos = i.pos + right + up;
    g4.pos = i.pos + right - up;
    
    g1.pos_local = float2(-2, -2);
    g2.pos_local = float2(-2,  2);
    g3.pos_local = float2( 2,  2);
    g4.pos_local = float2( 2, -2);
    
    g1.col = i.col;
    g2.col = i.col;
    g3.col = i.col;
    g4.col = i.col;
    
    o.Append(g1);
    o.Append(g2);
    o.Append(g3);
    o.RestartStrip();
    o.Append(g1);
    o.Append(g3);
    o.Append(g4);
}

//Pixel Shader. Determines final color and alpha
float4 pixelShader(GOut i) : SV_TARGET
{
    float d = dot(i.pos_local, i.pos_local);
    float alpha = i.col.a * exp(-d);
    return float4(i.col.rgb * alpha, alpha);
}