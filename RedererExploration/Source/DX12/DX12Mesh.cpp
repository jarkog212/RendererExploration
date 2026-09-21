#include "DX12Mesh.h"
#include "DX12UploadRequest.h"

#include "DirectXMath.h"

#include <mutex>

namespace
{
	struct MeshData
	{
		ID3D12Resource* _resource = nullptr; //non-owning, deferred release;

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

		void Release() 
		{
			Utils::COM::SafeDeferredRenderThreadRelease(_resource);
			*this = {};
		}
	};

	std::vector<MeshData> g_MeshDataCollection;
	std::vector<int> g_IDPool;
	unsigned int g_currentIDPoolIndex = 0;

	// the following vectors are expected to have parity (i.e. same size and correlating indeces)
	std::mutex g_mutex;

	void BatchResizeMeshDataCollection()
	{
		::g_IDPool.resize(::g_IDPool.size() + DX12::Meshes::k_IdPoolBatchReserveSize);
		::g_MeshDataCollection.resize(::g_MeshDataCollection.size() + DX12::Meshes::k_IdPoolBatchReserveSize);

		for (int i = 0; i < DX12::Meshes::k_IdPoolBatchReserveSize; i++)
			::g_IDPool[::g_currentIDPoolIndex + i] = ::g_currentIDPoolIndex + i;
	}
}

int DX12::Meshes::Add(const void* stream, unsigned long long& consumedBytes)
{
	CRITICAL_ASSERT(stream, "Invalid mesh stream.");

	const char* byteStream = reinterpret_cast<const char*>(stream);
	const DX12::Meshes::MeshStreamHeader* header = reinterpret_cast<const DX12::Meshes::MeshStreamHeader*>(stream);

	constexpr unsigned int headerSize = sizeof(MeshStreamHeader);
	constexpr unsigned int alignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

	const unsigned int indexSize = header->_vertexCount < (1 << 16) ? sizeof(unsigned short) : sizeof(unsigned int);
	const unsigned int positionBuffferSize = header->_vertexCount * sizeof(DirectX::XMFLOAT3);
	const unsigned int alignedPositionBuffferSize = Utils::AlignSizeUp<alignment>(positionBuffferSize);
	const unsigned int elementBufferSize = header->_elementSize * header->_vertexCount;
	const unsigned int alignedElementBufferSize = Utils::AlignSizeUp<alignment>(elementBufferSize);
	const unsigned int indexBufferSize = header->_indexCount * indexSize;

	// the 3 buffers aren't tightly packed (unlike the header) so we added the padding to the first 2 sub buffers.
	const unsigned long long int totalBufferSize = alignedPositionBuffferSize +	alignedElementBufferSize + indexBufferSize;
	consumedBytes = headerSize + totalBufferSize;

	FALLBACK_ASSERT(
		header->_primitiveTopology > DX12::Helpers::ConverterSupportedTopologies::Unknown
		&& header->_primitiveTopology < DX12::Helpers::ConverterSupportedTopologies::Count, ,
		return -1,
		"Stream contains a mesh with unsupported topology.");

	ID3D12Resource* resource = DX12::UploadDataToGPU(byteStream + headerSize, totalBufferSize);
	int meshCollectionIndex = -1;

	{
		std::lock_guard lock(::g_mutex);

		if (::g_currentIDPoolIndex >= ::g_IDPool.size())
			::BatchResizeMeshDataCollection();

		meshCollectionIndex = ::g_IDPool[::g_currentIDPoolIndex++];
		MeshData& meshData = ::g_MeshDataCollection[meshCollectionIndex];

		meshData._resource = resource;

		meshData._positionView._positionView.BufferLocation = resource->GetGPUVirtualAddress();
		meshData._positionView._positionView.SizeInBytes = positionBuffferSize;
		meshData._positionView._positionView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		meshData._positionView._indexView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBuffferSize + alignedElementBufferSize;
		meshData._positionView._indexView.Format = indexSize == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		meshData._positionView._indexView.SizeInBytes = indexBufferSize;

		if (header->_elementSize > 0)
		{
			meshData._elementsView._vertexView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBuffferSize;
			meshData._elementsView._vertexView.SizeInBytes = elementBufferSize;
			meshData._elementsView._vertexView.StrideInBytes = header->_elementSize;
		}

		meshData._elementsView._elementtype = header->_elementsType;
		meshData._elementsView._topology = (D3D_PRIMITIVE_TOPOLOGY)header->_primitiveTopology;
	}

	return meshCollectionIndex;
}

void DX12::Meshes::Remove(unsigned int index)
{
	std::lock_guard lock(::g_mutex);
	::g_IDPool[--::g_currentIDPoolIndex] = index;
	::g_MeshDataCollection[index].Release();
}

void DX12::Meshes::WarmUp()
{
	::BatchResizeMeshDataCollection();
}

void DX12::Meshes::Release()
{
	for (auto& it : ::g_MeshDataCollection)
		it.Release();

	::g_MeshDataCollection.clear();
	::g_IDPool.clear();
	::g_currentIDPoolIndex = 0;
}
