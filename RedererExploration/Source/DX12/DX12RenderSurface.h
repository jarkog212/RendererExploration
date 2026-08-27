#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class RendererWorker;

	class RenderSurface
	{
	public:

		RenderSurface(const HWND window, DXGI_FORMAT format, DX12::RendererWorker& rendererWorker);
		DEBUG_DESTRUCTOR(RenderSurface)
		RenderSurface(RenderSurface&& other) noexcept;
		RenderSurface& operator=(RenderSurface&& other) noexcept;

		NO_COPY(RenderSurface);

		void Resize();
		void Present();

		void ResetSwapChain(DX12::RendererWorker& rendererCommand, DXGI_FORMAT format);

		constexpr HWND GetWindow() const { return _window; }
		RECT GetWindowDimensions() const;
		constexpr unsigned int GetWidth() const { return _viewport.Width; }
		constexpr unsigned int GetHeight() const { return _viewport.Height; }
		ID3D12Resource* GetBackbuffer() const { return _renderTargetData[_currentBackBufferIndex]._resource.Get(); }
		DX12::DescriptorHandleView GetBackbufferHandle() const { return _renderTargetData[_currentBackBufferIndex]._rtvHandle.CreateView(); }
		constexpr const D3D12_VIEWPORT& GetViewport() const { return _viewport; }
		constexpr const D3D12_RECT& GetScissor() const { return _scissorRect; }

	private:

		void SetupRenderTargets();
		void PurgeRenderTargets();
		void UnlinkRenderTargetsResources();

		HWND _window = NULL;
		mutable unsigned int _currentBackBufferIndex = 0;
		D3D12_VIEWPORT _viewport = {};
		D3D12_RECT _scissorRect = {};
		unsigned int _presentFlags = 0;
		unsigned int _allowTearing = 0;
		DXGI_FORMAT _suppliedSurfaceFormat = DXGI_FORMAT_UNKNOWN;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
		DX12::RenderTargetData _renderTargetData[DX12::CC::k_swapChainBuffers];
	};
}