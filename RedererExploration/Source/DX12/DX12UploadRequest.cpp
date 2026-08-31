#include "DX12UploadRequest.h"
#include "DX12Renderer.h"
#include "DX12Utils.h"

ID3D12Resource* DX12::UploadDataToGPU(const void* data, unsigned int bufferSize, bool isCPUAccessible, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, ID3D12Heap* heap, unsigned long long int heapOffset, UploadCopyWorker* uploadCopyWorker)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::BufferResource::k_bufferTemplate;
	desc.Width = bufferSize;
	desc.Flags = isCPUAccessible ? D3D12_RESOURCE_FLAG_NONE : flags;

	CRITICAL_ASSERT(
		desc.Flags == D3D12_RESOURCE_FLAG_NONE || desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		"Created resource is not intended for upload use.");

	ID3D12Resource* resource = nullptr;
	const D3D12_RESOURCE_STATES resourceState = isCPUAccessible ? D3D12_RESOURCE_STATE_GENERIC_READ : state;

	if (heap)
	{
		HRESULT_CHECK_CRITICAL(
			DX12::GetRendererDevice()->CreatePlacedResource(
				heap,
				heapOffset,
				&desc,
				resourceState,
				nullptr,
				IID_PPV_ARGS(&resource)),
			"Could not create a placed buffer resource.");
	}
	else
	{
		HRESULT_CHECK_CRITICAL(
			DX12::GetRendererDevice()->CreateCommittedResource(
				isCPUAccessible ? &DX12::CC::Desc::Heap::k_uploadTemplate : &DX12::CC::Desc::Heap::k_baseTemplate,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				resourceState,
				nullptr,
				IID_PPV_ARGS(&resource)),
			"Could not create a committed texture resource.");
	}

	if (data)
	{
		if (isCPUAccessible)
		{
			D3D12_RANGE range{}; // init/nulled state implies no read access 
			void* address = nullptr;
			HRESULT_CHECK_CRITICAL(
				resource->Map(0, &range, (void**)&address),
				"Failed to map created resource.");

			memcpy(address, data, bufferSize);
			resource->Unmap(0, nullptr);
		}
		else
		{
			CRITICAL_ASSERT(uploadCopyWorker, "Upload/copy data via proxy buffer requires a copy/upload worker.")
			DX12::GPUUploadRequest request(bufferSize, data, *uploadCopyWorker);
			request.ApplyCopy(resource);
			request.EndRequest();
		}
	}

	return resource;
}

DX12::GPUUploadRequest::GPUUploadRequest(unsigned int alignedSize, const void* data, UploadCopyWorker& uploadCopyWorker)
{
	CRITICAL_ASSERT(data, "Cannot make upload context without valid data to upload.");

	_uploadCopyWorker = &uploadCopyWorker;
	_frameId = _uploadCopyWorker->ReserveUploadFrame();
	const DX12::UploadCopyWorker::UploadCopyFrame& frameView = _uploadCopyWorker->GetFrameConst(_frameId);

	ID3D12Resource* resource = DX12::UploadDataToGPU(data, alignedSize, true);
	wchar_t buffer[250]{};
	swprintf(buffer, 250, L"Proxy Upload Buffer (size: %u)", alignedSize);
	HRESULT_CHECK_CRITICAL(resource->SetName(buffer), "Could not name upload buffer.");

	_uploadCopyWorker->AssignBuffer(_frameId, std::move(resource));
	_resource = frameView.GetResource();
	_commandList = frameView.GetCommandList();

	ID3D12CommandAllocator* allocator = frameView.GetAllocator();
	HRESULT_CHECK_CRITICAL(allocator->Reset(), "COuld not reset upload buffer allocator.");
	_commandList->Reset(allocator, nullptr);
}

void DX12::GPUUploadRequest::EndRequest()
{
	HRESULT_CHECK_CRITICAL(_commandList->Close(), "Could not Close() upload command list");
	_uploadCopyWorker->ExecuteCopy(_frameId);
}

void DX12::GPUUploadRequest::ApplyCopy(ID3D12Resource*& GPUOnlyResource)
{
	_commandList->CopyResource(GPUOnlyResource, _resource);
}
