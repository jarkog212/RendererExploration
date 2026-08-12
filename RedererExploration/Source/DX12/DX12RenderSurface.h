#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class RenderSurface
	{
	public:

		RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue* commandQueue);
		~RenderSurface() { Release(); }
		RenderSurface(RenderSurface&& other) noexcept;
		RenderSurface& operator=(RenderSurface&& other) noexcept;

		NO_COPY(RenderSurface);

		void Resize();
		void Present() const;

		constexpr HWND GetWindow() const { return _window; }
		RECT GetWindowDimensions() const;
		constexpr unsigned int GetWidth() const { return _viewport.Width; }
		constexpr unsigned int GetHeight() const { return _viewport.Height; }
		ID3D12Resource* GetBackbuffer() const { return _renderTargetData[_currentBackBufferIndex]._resource.Get(); }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetBackbufferCPUHandle() const { return _renderTargetData[_currentBackBufferIndex]._rtvHandle._cpu; }
		constexpr const D3D12_VIEWPORT& GetViewport() const { return _viewport; }
		constexpr const D3D12_RECT& GetScissor() const { return _scissorRect; }

	private:

		void Release();

		void Finalize(DXGI_FORMAT format);

		static constexpr int _k_swapChainBuffers = 3;

		Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
		HWND _window = NULL;
		mutable unsigned int _currentBackBufferIndex = 0;
		D3D12_VIEWPORT _viewport = {};
		D3D12_RECT _scissorRect = {};
		unsigned int _presentFlags = 0;
		unsigned int _allowTearing = 0;
		RenderTargetData _renderTargetData[DX12::CC::k_frameBuffers];
	};
}