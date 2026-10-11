#include "DX12Mesh.h"
#include "DX12UploadRequest.h"
#include "DX12DescriptorHeap.h"

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

DX12::Meshes::WorldTransformBuffer::Allocation::Allocation(int index, DirectX::XMMATRIX& matrix, WorldTransformBuffer& owner) :
	_index(index),
	_transformationMatrix(&matrix),
	_worldTransformBufferOwner(&owner)
{
}

DX12::Meshes::WorldTransformBuffer::Allocation::~Allocation()
{
	DEBUG_DESTRUCTOR_BODY(Allocation);

	Utils::SafeDeferredRenderThreadReleaseViaLambda([bufferOwner = _worldTransformBufferOwner, index = _index]()
		{
			bufferOwner->Release(index);
		});
}

DirectX::XMMATRIX& DX12::Meshes::WorldTransformBuffer::Allocation::GetTransformMatrix()
{
	CRITICAL_ASSERT(_index != _k_Inavlid, "Trying to access invalid matrix.");
	return _worldTransformBufferOwner->GetWorldTransformMatrix(_index);
}

DX12::Meshes::WorldTransformBuffer::WorldTransformBuffer()
{
	ReserveBatch();
	Sync(DX12::CC::k_forAllFrames);
}

int DX12::Meshes::WorldTransformBuffer::GetMatrixSRVIndex(int currentFrame)
{
	// Hail Mary just in time sync to GPU... should be done elsewhere
	if (!_perFrameData[currentFrame]._srvHandleTransforms.IsValid())
		Sync(currentFrame);

	return _perFrameData[currentFrame]._srvHandleTransforms._index;
}

int DX12::Meshes::WorldTransformBuffer::GetIDSRVIndex(int currentFrame)
{
	// Hail Mary just in time sync to GPU... should be done elsewhere
	if (!_perFrameData[currentFrame]._srvHandleIds.IsValid())
		Sync(currentFrame);

	return _perFrameData[currentFrame]._srvHandleIds._index;
}

void DX12::Meshes::WorldTransformBuffer::Sync(int currentFrame)
{
	static constexpr int elementSize = sizeof(DirectX::XMMATRIX);
	static constexpr int elementIdSize = sizeof(int);

	static auto syncer = [&](int currentFrame) 
		{
			PerFrameData& data = _perFrameData[currentFrame];
			if (!data._meshWorldTransforms)
			{
				data._meshWorldTransforms = DX12::UploadDataToNewGPUBuffer(_stagingBuffer.data(), _stagingBuffer.size() * elementSize);

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::BufferResource::k_StructuredSRVTemplate;
				srvDesc.Buffer.NumElements = _stagingBuffer.size();
				srvDesc.Buffer.StructureByteStride = elementSize;

				data._srvHandleTransforms = DX12::GetSRVHeap()->Allocate();
				DX12::GetRendererDevice()->CreateShaderResourceView(data._meshWorldTransforms, &srvDesc, data._srvHandleTransforms._cpu);
			}
			else
			{
				DX12::UploadDataToGPU(data._meshWorldTransforms, _stagingBuffer.data(), _stagingBuffer.size() * elementSize);
			}

			if (data._dirtyIDs)
			{
				if (data._meshWorldTransformsIDs)
				{
					data._meshWorldTransformsIDs = DX12::UploadDataToNewGPUBuffer(_assignedIndeces.data(), _assignedIndeces.size() * elementIdSize);

					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::BufferResource::k_StructuredSRVTemplate;
					srvDesc.Buffer.NumElements = _assignedIndeces.size();
					srvDesc.Buffer.StructureByteStride = elementIdSize;

					data._srvHandleIds = DX12::GetSRVHeap()->Allocate();
					DX12::GetRendererDevice()->CreateShaderResourceView(data._meshWorldTransformsIDs, &srvDesc, data._srvHandleIds._cpu);
				}
				else
				{
					DX12::UploadDataToGPU(data._meshWorldTransformsIDs, _assignedIndeces.data(), _assignedIndeces.size() * elementIdSize);
				}

				data._dirtyIDs = false;
			}
		};

	if (currentFrame == DX12::CC::k_forAllFrames)
	{
		for (int i = 0; i <= DX12::CC::k_rendererWorkerFrameBuffers; i++)
			syncer(i);
	}
	else 
	{
		syncer(currentFrame);
	}
}

DX12::Meshes::WorldTransformBuffer::Allocation DX12::Meshes::WorldTransformBuffer::Allocate()
{
	if (_upcomingSlot == _assignIndexQueue.size())
		ReserveBatch();
	
	const int index = _assignIndexQueue[_upcomingSlot++];
	CRITICAL_ASSERT(_assignedIndecesTracker[index] == -1, "Some meshe's world transform buffer is allocatting a matrix which is supposedly in use.");

	_assignedIndeces.push_back(index);

	for (int i = 0; i <= DX12::CC::k_rendererWorkerFrameBuffers; i++)
		_perFrameData[i]._dirtyIDs = true;

	_assignedIndecesTracker[index] = _assignedIndeces.size() - 1;
	return DX12::Meshes::WorldTransformBuffer::Allocation(index, _stagingBuffer[index], *this);
}

void DX12::Meshes::WorldTransformBuffer::Release(int index)
{
	int slot = _assignedIndecesTracker[index];
	CRITICAL_ASSERT(slot != -1 && _assignedIndeces[slot] == index, "Releasing a slot which is wa released before (or is not correct. i.e. double-free)");

	int replacement = _assignedIndeces[slot] = _assignedIndeces[_assignedIndeces.size() - 1];
	_assignedIndeces.pop_back();
	
	for (int i = 0; i <= DX12::CC::k_rendererWorkerFrameBuffers; i++)
		_perFrameData[i]._dirtyIDs = true;

	_assignedIndecesTracker[replacement] = slot;
	_assignedIndecesTracker[index] = DX12::Meshes::WorldTransformBuffer::Allocation::_k_Inavlid;
	_assignIndexQueue[--_upcomingSlot] = index;
}

DirectX::XMMATRIX& DX12::Meshes::WorldTransformBuffer::GetWorldTransformMatrix(int index)
{
	return _stagingBuffer[index];
}

void DX12::Meshes::WorldTransformBuffer::ReserveBatch()
{
	_stagingBuffer.resize(_stagingBuffer.size() + _k_batchAllocation);
	_assignIndexQueue.resize(_assignIndexQueue.size() + _k_batchAllocation);
	_assignedIndeces.reserve(_assignedIndeces.size() + _k_batchAllocation);
	_assignedIndecesTracker.resize(_assignedIndeces.size() + _k_batchAllocation, -1);
	
	for (int i = _upcomingSlot; i < _assignIndexQueue.size(); i++)
		_assignIndexQueue[i] = i;

	for (int i = 0; i <= DX12::CC::k_rendererWorkerFrameBuffers; i++)
		_perFrameData[i] = PerFrameData();
}

DX12::Meshes::WorldTransformBuffer::PerFrameData::~PerFrameData()
{
	Utils::COM::SafeDeferredRenderThreadRelease(_meshWorldTransforms);
	Utils::COM::SafeDeferredRenderThreadRelease(_meshWorldTransformsIDs);
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
	_meshTemplateBuffer = DX12::UploadDataToNewGPUBuffer(byteStream + headerSize, totalBufferSize);

	_positionView._positionView.BufferLocation = _meshTemplateBuffer->GetGPUVirtualAddress();
	_positionView._positionView.SizeInBytes = positionBuffferSize;
	_positionView._positionView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
	_positionView._indexView.BufferLocation = _meshTemplateBuffer->GetGPUVirtualAddress() + alignedPositionBuffferSize + alignedElementBufferSize;
	_positionView._indexView.Format = indexSize == sizeof(unsigned short) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	_positionView._indexView.SizeInBytes = indexBufferSize;

	if (header._elementSize > 0)
	{
		_elementsView._vertexView.BufferLocation = _meshTemplateBuffer->GetGPUVirtualAddress() + alignedPositionBuffferSize;
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
	Utils::COM::SafeDeferredRenderThreadRelease(_meshTemplateBuffer);
}

DX12::Meshes::WorldTransformBuffer::Allocation DX12::Meshes::Mesh::ReserveTransformMatrix()
{
	return _instancesWorldTransforms.Allocate();
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
		consumedBytes = mesh->GetTotalSize();
		return mesh;
	}
}