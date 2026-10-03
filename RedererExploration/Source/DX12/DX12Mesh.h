#pragma once
#include "DX12Utils.h"

namespace DX12::Meshes
{
    // ensure this is tightly packed (used to map onto the stream header)
#pragma pack(push, 1)

    /* data format expectation of a single stream segment
        Header:
            u32 elementSize,
            u32 vertexCount,
            u32 indexCount,
            u32 elementType,
            u32 primitiveTopology
        Buffers (aligned to 16 bytes):
            char* positions[vertexCount * sizeof(DX12::Math::Vector3f)],
            char* elements[vertexCount * elementSize]
            char* indices[indexCount * indexSize]

        * note indexSize is dependent on the number of indeces (either 16 or 32 bytes)
    */

    struct StreamHeader {
        unsigned int _elementSize;
        unsigned int _vertexCount;
        unsigned int _indexCount;
        unsigned int _elementsType;
        DX12::Helpers::ConverterSupportedTopologies _primitiveTopology;
    };

#pragma pack(pop)

    struct MeshView
    {
        ID3D12Resource* _resource = nullptr;

        struct PositionView
        {
            D3D12_VERTEX_BUFFER_VIEW _positionView;
            D3D12_INDEX_BUFFER_VIEW _indexView;
        }_positionView;

        struct ElementView
        {
            D3D12_VERTEX_BUFFER_VIEW _vertexView;
            unsigned int _elementtype;
            D3D_PRIMITIVE_TOPOLOGY _topology;

        }_elementsView;
    };

    struct Mesh : public MeshView
    {
        Mesh(const wchar_t* name, const char* byteStream, const DX12::Meshes::StreamHeader& header);
        ~Mesh();
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        NO_COPY(Mesh);

        const DX12::Meshes::MeshView CreateView() const { return { _resource, _positionView, _elementsView }; };

        int _totalSize = -1;
        std::wstring _name = nullptr;
    };
}

namespace DX12::MeshSpace
{
    void Release();
    void WarmUp();

    std::shared_ptr<DX12::Meshes::Mesh> GetOrLoad(const wchar_t* name, void* stream, unsigned long long& consumedBytes);
}