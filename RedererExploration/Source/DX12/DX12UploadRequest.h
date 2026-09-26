#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"

namespace DX12
{
	ID3D12Resource* UploadDataToNewGPUBuffer(
		const void* data,
		unsigned int bufferSize,
		D3D12_RESOURCE_DESC* desc = nullptr,
		bool isCPUAccessible = false,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		ID3D12Heap* heap = nullptr,
		unsigned long long int heapOffset = 0,
		UploadCopyWorker* uploadCopyWorker = DX12::GetMainUploadCopyWorker());

	class GPUUploadRequest
	{
	public:

		GPUUploadRequest(
			unsigned int alignedSize,
			const void* data,
			D3D12_RESOURCE_DESC* desc = nullptr,
			UploadCopyWorker& uploadCopyWorker = *DX12::GetMainUploadCopyWorker());

		DEBUG_DESTRUCTOR(GPUUploadRequest);

		NO_COPY(GPUUploadRequest);
		NO_MOVE(GPUUploadRequest);

		void EndRequest();
		void ApplyCopy(ID3D12Resource*& GPUOnlyResource);

	private:

		ID3D12GraphicsCommandList6* _commandList = nullptr; //non-owning
		ID3D12Resource* _resource = nullptr; // non-owning
		UploadCopyWorker* _uploadCopyWorker = nullptr; //non-owning
		unsigned int _frameId = 0;
		unsigned int _remainigSpace = 0;
	};
}