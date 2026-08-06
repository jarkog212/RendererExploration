#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace DX12
{
	namespace CC 
	{
		constexpr DXGI_FORMAT k_defaultRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	class GfxCommand;
	class RenderSurface;
	class DescriptorHeap;

	ID3D12Device8* GetRendererDevice();
	IDXGIFactory7* GetDxgiFactory();
	IDXGIAdapter4* GetDxgiAdapter();

	DX12::RenderSurface* GetMainRenderSurface(
		HWND window = NULL,
		DXGI_FORMAT format = DX12::CC::k_defaultRenderTargetFormat,
		ID3D12CommandQueue* commandQueue = nullptr);

	DX12::GfxCommand* GetMainRenderCommand();
	DX12::DescriptorHeap* GetRTVHeap();
	DX12::DescriptorHeap* GetDSVHeap();
	DX12::DescriptorHeap* GetSRVHeap();
	DX12::DescriptorHeap* GetUAVHeap();

	void SetPendingResourceFreeingFlag();
	bool TestPendingResourceFreeingFlag(const unsigned int frame);

	bool Initialize(HWND hwnd);
	void Render(DX12::GfxCommand* command, DX12::RenderSurface* surface);
	void Render();
	void Cleanup();
}