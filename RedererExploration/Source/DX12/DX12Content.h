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

    int Add(const void* stream, unsigned int& consumedBytes);
    void Remove(unsigned int index);
    void Release() {};
}