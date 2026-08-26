#pragma once

#include "DX12CommonConstants.h"
#include "Utils.h"

namespace DX12
{
	class GPUUploadContext;

	inline ID3D12Resource* CreateDataBuffer(
		unsigned int bufferSize,
		void* data = nullptr,
		bool isCPUAccessible = false,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE,
		ID3D12Heap* heap = nullptr,
		unsigned long long int heapOffset = 0)
	{
		D3D12_RESOURCE_DESC desc = DX12::CC::Desc::BufferResource::k_bufferTemplate;
		desc.Width = bufferSize;
		desc.Flags = isCPUAccessible ? D3D12_RESOURCE_FLAG_NONE : flags;

		CRITICAL_ASSERT(
			desc.Flags == D3D12_RESOURCE_FLAG_NONE || desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			"Created resource is not intended for upload use.");

		ID3D12Resource* out = nullptr;
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
					IID_PPV_ARGS(&out)),
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
					IID_PPV_ARGS(&out)),
				"Could not create a committed texture resource.");
		}

		if (data)
		{
			if (isCPUAccessible)
			{
				D3D12_RANGE range{}; // init/nulled state implies no read access 
				void* address = nullptr;
				HRESULT_CHECK_CRITICAL(
					out->Map(0, &range, (void**)&address),
					"Failed to map created resource.");

				memcpy(address, data, bufferSize);

				out->Unmap(0, nullptr);
			}
			else
			{
				DX12::GPUUploadContext context(bufferSize);
			}
		}

		return out;
	}

	class GPUUploadContext 
	{
	public:
	
		GPUUploadContext(unsigned int alignedSize);
		~GPUUploadContext() { DEBUG_DESTRUCTOR_BODY(GPUUploadContext) }

		NO_COPY(GPUUploadContext);
		NO_MOVE(GPUUploadContext);

	private:

		ID3D12CommandList* _commandList = nullptr; //non-owning
		unsigned int _frameId = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
		void* _address = nullptr;
	};
}