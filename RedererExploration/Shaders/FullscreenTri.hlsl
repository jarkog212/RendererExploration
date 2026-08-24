struct VSOutput
{
    noperspective float4 _position: SV_Position;
    noperspective float2 _UVs: TEXCOORD;
};

VSOutput FullscreenTri (in uint vertexIdx : SV_VertexID)
{
    VSOutput output;

    const float2 tex = float2(uint2(vertexIdx, vertexIdx << 1) & 2);
    output._position = float4(lerp(float2(-1, 1), float2(1, -1), tex), 0, 1);
    output._UVs = tex;

    return output;
}