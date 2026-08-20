struct MandelbrotPostProcessConstants
{
    float _width;
    float _height;
    uint _frame;
};

ConstantBuffer<MandelbrotPostProcessConstants> g_constants : register(b0);

#define M_RE_START -2.8
#define M_IM_START -1.5
#define M_RE_END 1.f
#define M_IM_END 1.5f
#define M_MAX_ITERATION 1000

#define M_MAX_DIVERGENCE_LEN_POW2 4.f

#define M_RE_RANGE M_RE_END-M_RE_START
#define M_IM_RANGE M_IM_END-M_IM_START

#define M_AMBIENT float3(0.09f, 0.12f, 0.16f)

float2 ComplexPow2(float2 c)
{
    return float2(c.x * c.x - c.y * c.y, 2.f * c.x * c.y);
}

float3 MapColour(float value)
{
    return M_AMBIENT + float3(3.f * value, 5.f * value, 10.f * value);
}

float3 DrawMandelbrot(float2 uv)
{
    const float2 c = float2(M_RE_START + uv.x * (M_RE_RANGE), M_IM_START + uv.y * (M_IM_RANGE));

    float2 z = 0.f;
    for(int i = 0; i < M_MAX_ITERATION; i++)
    {
        z = ComplexPow2(z) + c;
        const float d = dot(z, z);
        if (d > M_MAX_DIVERGENCE_LEN_POW2)
            return MapColour((float)i / M_MAX_ITERATION);
    }

    return 1.f;
}

float4 Mandelbrot (
    in noperspective float4 position : SV_Position,
    in noperspective float2 UVs : TEXCOORD) : SV_Target0
{

    const float2 invertedDimensions = float2(1.f / g_constants._width, 1.f / g_constants._height);
    const float2 uv = (position.xy) * invertedDimensions;

    return float4(DrawMandelbrot(uv), 1.f);
}