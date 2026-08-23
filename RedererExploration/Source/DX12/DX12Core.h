#pragma once

#include <utility>

#include "DX12CommonConstants.h"
#include <wrl.h>

namespace DX12
{
	class RendererCommand;
	class RenderSurface;
	class DescriptorHeap;
	class RenderPassBase;

	ID3D12Device8* GetRendererDevice();
	IDXGIFactory7* GetDxgiFactory();
	IDXGIAdapter4* GetDxgiAdapter();

	DX12::RenderSurface* GetMainRenderSurface(
		HWND window = NULL,
		DXGI_FORMAT format = DX12::CC::k_defaultRenderTargetFormat,
		DX12::RendererCommand* renderer = nullptr);

	DX12::RendererCommand* GetMainRendererCommand();
	DX12::DescriptorHeap* GetRTVHeap();
	DX12::DescriptorHeap* GetDSVHeap();
	DX12::DescriptorHeap* GetSRVHeap();
	DX12::DescriptorHeap* GetUAVHeap();

	void SetPendingResourceFreeingFlag();
	bool TestPendingResourceFreeingFlag(const unsigned int frame);

	void PendingTransitionResourceBarrier(
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES before,
		D3D12_RESOURCE_STATES after,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		unsigned int subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void PendingUAVResourceBarrier(
		ID3D12Resource* resource,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

	void PendingAliasingResourceBarrier(
		ID3D12Resource* resourceBefore,
		ID3D12Resource* resourceAfter,
		D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

	void SubmitPendingBarriers(ID3D12GraphicsCommandList6* commandList);

	bool Initialize(HWND hwnd);
	void Render(DX12::RendererCommand& rendererCommand);
	void Render();
	void ReleaseAll();

#ifdef _DEBUG

	namespace DEBUG {

		void PrintDeviceRemovedReason(HRESULT hr);
	}

#endif
}