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

	bool Initialize(HWND hwnd);
	void Render(DX12::RenderSurface& surface, DX12::RenderPassBase* root = nullptr );
	void Render();
	void Release();
}