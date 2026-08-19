struct PostProcessConstants
{
    uint _inputBufferIndex;
};

ConstantBuffer<PostProcessConstants> g_constants : register(b0);
Texture2D g_SRVHeap[] : register(t0, space0);

float4 PostProcess (
    in noperspective float4 position : SV_Position,
    in noperspective float2 UVs : TEXCOORD) : SV_Target0
{
    Texture2D inputTexture = g_SRVHeap[g_constants._inputBufferIndex];
    float4 color = float4(inputTexture[position.xy].xyz, 1.f);

    //return color;
    return float4(0.5f, 0.f, 0.4f, 1.f);
}