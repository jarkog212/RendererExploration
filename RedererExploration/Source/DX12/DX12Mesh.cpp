#include "DX12Mesh.h"
#include "DX12UploadRequest.h"

#include "DirectXMath.h"

#include <mutex>

namespace
{
	// the following vectors are expected to have parity (i.e. same size and correlating indeces)
	std::unordered_map<std::wstring, std::weak_ptr<DX12::Meshes::Mesh>> g_meshes;

	std::mutex g_meshMutex;

	bool SafeReleaseMesh(const wchar_t* name)
	{
		std::lock_guard lock (::g_meshMutex);

		auto it = ::g_meshes.find(name);
		if (it == ::g_meshes.end())
			return false;

		CRITICAL_ASSERT(
			it->second.expired(),
			"The mesh to be released still has legitimate references. %ls",
			name);

		::g_meshes.erase(it);
		return true;
	}
}

DX12::Meshes::Mesh::Mesh(const wchar_t* name, const char* byteStream, const DX12::Meshes::StreamHeader& header)
{
	constexpr unsigned int headerSize = sizeof(DX12::Meshes::StreamHeader);
	constexpr unsigned int alignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;

	const unsigned int indexSize = header._vertexCount < (1 << 16) ? sizeof(unsigned short) : sizeof(unsigned int);
	const unsigned int positionBuffferSize = header._vertexCount * sizeof(DirectX::XMFLOAT3);
	const unsigned int alignedPositionBuffferSize = Utils::AlignSizeUp<alignment>(positionBuffferSize);
	const unsigned int elementBufferSize = header._elementSize * header._vertexCount;
	const unsigned int alignedElementBufferSize = Utils::AlignSizeUp<alignment>(elementBufferSize);
	const unsigned int indexBufferSize = header._indexCount * indexSize;

	FALLBACK_ASSERT(
		header._primitiveTopology > DX12::Helpers::ConverterSupportedTopologies::Unknown
		&& header._primitiveTopology < DX12::Helpers::ConverterSupportedTopologies::Count, ,
		return,
		"Stream contains a mesh with unsupported topology.");

	// the 3 buffers aren't tightly packed (unlike the header) so we added the padding to the first 2 sub buffers.
	const unsigned long long int totalBufferSize = alignedPositionBuffferSize + alignedElementBufferSize + indexBufferSize;
	_totalSize = headerSize + totalBufferSize;
	_resource = DX12::UploadDataToNewGPUBuffer(byteStream + headerSize, totalBufferSize);

	_positionView._positionView.BufferLocation = _resource->GetGPUVirtualAddress();
	_positionView._positionView.SizeInBytes = positionBuffferSize;
	_positionView._positionView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
	_positionView._indexView.BufferLocation = _resource->GetGPUVirtualAddress() + alignedPositionBuffferSize + alignedElementBufferSize;
	_positionView._indexView.Format = indexSize == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	_positionView._indexView.SizeInBytes = indexBufferSize;

	if (header._elementSize > 0)
	{
		_elementsView._vertexView.BufferLocation = _resource->GetGPUVirtualAddress() + alignedPositionBuffferSize;
		_elementsView._vertexView.SizeInBytes = elementBufferSize;
		_elementsView._vertexView.StrideInBytes = header._elementSize;
	}

	_elementsView._elementtype = header._elementsType;
	_elementsView._topology = (D3D_PRIMITIVE_TOPOLOGY)header._primitiveTopology;

	_name = name;
}

DX12::Meshes::Mesh::~Mesh()
{
	DEBUG_DESTRUCTOR_BODY(Mesh);
	::SafeReleaseMesh(_name.c_str());
	Utils::COM::SafeDeferredRenderThreadRelease(_resource);
}

void DX12::MeshSpace::WarmUp()
{
}

void DX12::MeshSpace::Release()
{
	std::lock_guard lock(::g_meshMutex);
	::g_meshes.clear();
}

std::shared_ptr<DX12::Meshes::Mesh> DX12::MeshSpace::GetOrLoad(const wchar_t* name, void* stream, unsigned long long& consumedBytes)
{
	CRITICAL_ASSERT(stream, "Invalid mesh stream.");
	CRITICAL_ASSERT(name, "Invalid Mesh name.");

	const char* byteStream = reinterpret_cast<const char*>(stream);
	const DX12::Meshes::StreamHeader* header = reinterpret_cast<const DX12::Meshes::StreamHeader*>(stream);

	{
		std::lock_guard lock(::g_meshMutex);

		if (auto it = ::g_meshes.find(name); it != ::g_meshes.end() && !it->second.expired()) [[likely]]
			return it->second.lock();

		std::shared_ptr<DX12::Meshes::Mesh> mesh = std::make_shared<DX12::Meshes::Mesh>(name, byteStream, *header);
		::g_meshes[name] = mesh;
		consumedBytes = mesh->_totalSize;
		return mesh;
	}
}
