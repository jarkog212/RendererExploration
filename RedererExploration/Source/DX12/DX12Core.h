#pragma once

#include <utility>

#include "DX12CommonConstants.h"
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace DX12
{
	class GfxCommandExecutor;
	class RenderSurface;
	class DescriptorHeap;
	class RootSignatureSpec;

	ID3D12Device8* GetRendererDevice();
	IDXGIFactory7* GetDxgiFactory();
	IDXGIAdapter4* GetDxgiAdapter();

	DX12::RenderSurface* GetMainRenderSurface(
		HWND window = NULL,
		DXGI_FORMAT format = DX12::CC::k_defaultRenderTargetFormat,
		ID3D12CommandQueue* commandQueue = nullptr);

	DX12::GfxCommandExecutor* GetMainCommandExecutor();
	DX12::DescriptorHeap* GetRTVHeap();
	DX12::DescriptorHeap* GetDSVHeap();
	DX12::DescriptorHeap* GetSRVHeap();
	DX12::DescriptorHeap* GetUAVHeap();

	ID3D12RootSignature* CreateRootSignature(const DX12::RootSignatureSpec& desc);
	ID3D12PipelineState* CreatePipelineState(void* stream, unsigned long long int streamSize);

	void SetPendingResourceFreeingFlag();
	bool TestPendingResourceFreeingFlag(const unsigned int frame);

	bool Initialize(HWND hwnd);
	void Render(DX12::GfxCommandExecutor* command, DX12::RenderSurface* surface);
	void Render();
	void Release();
}