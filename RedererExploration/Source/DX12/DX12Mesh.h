#pragma once

#include "DX12Utils.h"
#include "DX12Resources.h"

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

    class WorldTransformBuffer
    {
    public:

        class Allocation 
        {
        public:

            static constexpr int _k_Inavlid = -1;

            Allocation(int index, DirectX::XMMATRIX& matrix, WorldTransformBuffer& owner);
            ~Allocation();
            Allocation(Allocation&&) = default;
            Allocation& operator=(Allocation&&) = default;

            NO_COPY(Allocation);

            DirectX::XMMATRIX& GetTransformMatrix();

        private:

            DirectX::XMMATRIX* _transformationMatrix = nullptr;
            int _index = _k_Inavlid;
            WorldTransformBuffer* _worldTransformBufferOwner = nullptr;
        };

        WorldTransformBuffer();
        DEBUG_DESTRUCTOR(WorldTransformBuffer);
        WorldTransformBuffer(WorldTransformBuffer&&) = default;
        WorldTransformBuffer& operator=(WorldTransformBuffer&&) = default;

        NO_COPY(WorldTransformBuffer);

        int GetMatrixSRVIndex(int currentFrame);
        int GetIDSRVIndex(int currentFrame);
        void Sync(int currentFrame);
        Allocation Allocate();
        void Release(int index);
        int GetInstanceCount() { return _upcomingSlot; }
        DirectX::XMMATRIX& GetWorldTransformMatrix(int index);

    private:

        void ReserveBatch();

        static constexpr unsigned int _k_batchAllocation = 64;

        struct PerFrameData{

            PerFrameData() = default;
            ~PerFrameData();
            PerFrameData(PerFrameData&&) = default;
            PerFrameData& operator=(PerFrameData&&) = default;

            NO_COPY(PerFrameData);

            ID3D12Resource* _meshWorldTransforms = nullptr; // non-owning - deferred release
            DX12::DescriptorHandle _srvHandleTransforms = {};
            ID3D12Resource* _meshWorldTransformsIDs = nullptr; // non-owning - deferred release
            DX12::DescriptorHandle _srvHandleIds = {};
            bool _dirtyIDs = true;
        }_perFrameData[DX12::CC::k_rendererWorkerFrameBuffers];

        std::vector<DirectX::XMMATRIX> _stagingBuffer;
        std::vector<int> _assignIndexQueue;
        std::vector<int> _assignedIndeces;
        std::vector<int> _assignedIndecesTracker;
        int _upcomingSlot = 0;
    };

    struct MeshView
    {
        ID3D12Resource* _meshTemplateBuffer = nullptr; // non-owning - deferred release

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

    class Mesh : public MeshView
    {
    public:

        Mesh(const wchar_t* name, const char* byteStream, const DX12::Meshes::StreamHeader& header);
        ~Mesh();
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;

        NO_COPY(Mesh);

        const DX12::Meshes::MeshView CreateView() const { return { _meshTemplateBuffer, _positionView, _elementsView }; };
        WorldTransformBuffer::Allocation ReserveTransformMatrix();
        int GetInstanceWorldMatricesSRVIndex(int currentFrame) { _instancesWorldTransforms.GetMatrixSRVIndex(currentFrame); }
        int GetInstanceWorldMatrixIDsSRVIndex(int currentFrame) { _instancesWorldTransforms.GetIDSRVIndex(currentFrame); }
        int GetInstanceCount() { _instancesWorldTransforms.GetInstanceCount(); }
        int GetTotalSize() { return _totalSize; }

    private:

        int _totalSize = -1;
        std::wstring _name = nullptr;
        WorldTransformBuffer _instancesWorldTransforms;
    };
}

namespace DX12::MeshSpace
{
    void Release();
    void WarmUp();

    std::shared_ptr<DX12::Meshes::Mesh> GetOrLoad(const wchar_t* name, void* stream, unsigned long long& consumedBytes);
}
