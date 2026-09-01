#include "DX12Content.h"
#include "DX12Math.h"
#include "DX12UploadRequest.h"

#include <mutex>

namespace
{
	struct PositionView 
	{
		D3D12_VERTEX_BUFFER_VIEW _positionView;
		D3D12_INDEX_BUFFER_VIEW _indexView;
	};

	struct ElementView
	{
		D3D12_VERTEX_BUFFER_VIEW _vertexView;
		unsigned int _elementtype;
		D3D_PRIMITIVE_TOPOLOGY _topology;
	};

	// the following vectors are expected to have parity (i.e. same size and correlating indeces)
	std::vector<ID3D12Resource*> g_submeshResources; //non-owning, deferred release;
	std::vector<PositionView> g_positionViews;
	std::vector<ElementView> g_elements;
	std::mutex g_mutex;
}

int DX12::Meshes::Add(const void* stream, unsigned int& consumedBytes)
{
	const char* byteStream = reinterpret_cast<const char*>(stream);
	const DX12::Meshes::StreamHeader* header = reinterpret_cast<const DX12::Meshes::StreamHeader*>(stream);

	constexpr unsigned int headerSize = sizeof(StreamHeader);
	constexpr unsigned int alignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

	const unsigned int indexSize = header->_vertexCount < (1 << 16) ? sizeof(unsigned short) : sizeof(unsigned int);
	const unsigned int positionBuffferSize = header->_vertexCount * sizeof(DX12::Math::Vector3f);
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

	PositionView positionView;
	positionView._positionView.BufferLocation = resource->GetGPUVirtualAddress();
	positionView._positionView.SizeInBytes = positionBuffferSize;
	positionView._positionView.StrideInBytes = sizeof(DX12::Math::Vector3f);
	positionView._indexView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBuffferSize + alignedElementBufferSize;
	positionView._indexView.Format = indexBufferSize == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	positionView._indexView.SizeInBytes = indexBufferSize;

	ElementView elementsView;
	if (header->_elementSize > 0)
	{
		elementsView._vertexView.BufferLocation = resource->GetGPUVirtualAddress() + alignedPositionBuffferSize;
		elementsView._vertexView.SizeInBytes = elementBufferSize;
		elementsView._vertexView.StrideInBytes = header->_elementSize;
	}

	elementsView._elementtype = header->_elementsType;
	elementsView._topology = (D3D_PRIMITIVE_TOPOLOGY)header->_primitiveTopology;

	std::lock_guard lock(::g_mutex);

	::g_submeshResources.push_back(resource);
	::g_positionViews.push_back(positionView);
	::g_elements.push_back(elementsView);

	return ::g_submeshResources.size() - 1;
}

void DX12::Meshes::Remove(unsigned int index)
{
	std::lock_guard lock(::g_mutex);
	 // problem
	g_positionViews.erase(g_positionViews.begin() + index);
}
