#include "DX12Material.h"

#include <map>
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