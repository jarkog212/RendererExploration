struct VSOutput
{
    noperspective float4 _position: SV_Position;
    noperspective float2 _UVs: TEXCOORD;
};

VSOutput FullscreenTri (in uint vertexIdx : SV_VertexID)
{
    VSOutput output;

    float2 tex;
    float2 pos;

    output._position = float4(0,0,0,1);

    if (vertexIdx == 0)
    {
        tex = float2(0,0);
        pos = float2(-1,1);
    }
    else if(vertexIdx == 1)
    {
        tex = float2(0,2);
        pos = float2(-1, -3);
    }
    else if(vertexIdx == 2)
    {
        tex = float2(2,0);
        pos = float2(3, 1);
    }

    output._position = float4(pos, 0, 1);
    output._UVs = tex;

    return output;
}