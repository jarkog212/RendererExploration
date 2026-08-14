#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class RendererCommand;

	class RenderSurface
	{
	public:

		RenderSurface(const HWND window, DXGI_FORMAT format, DX12::RendererCommand& rendererCommand);
		~RenderSurface();
		RenderSurface(RenderSurface&& other) noexcept;
		RenderSurface& operator=(RenderSurface&& other) noexcept;

		NO_COPY(RenderSurface);

		void Resize();
		void PreRender();
		void PostRender();
		void UpdateViewport();

		void ResetSwapChain(DX12::RendererCommand& rendererCommand, DXGI_FORMAT format);

		DX12::RendererCommand* GetLinkedRendererCommand() const { return _linkedRendererCommand; }
		constexpr HWND GetWindow() const { return _window; }
		RECT GetWindowDimensions() const;
		constexpr unsigned int GetWidth() const { return _viewport.Width; }
		constexpr unsigned int GetHeight() const { return _viewport.Height; }
		ID3D12Resource* GetBackbuffer() const { return _renderTargetData[_currentBackBufferIndex]._resource.Get(); }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetBackbufferCPUHandle() const { return _renderTargetData[_currentBackBufferIndex]._rtvHandle._cpu; }
		constexpr const D3D12_VIEWPORT& GetViewport() const { return _viewport; }
		constexpr const D3D12_RECT& GetScissor() const { return _scissorRect; }

	private:

		void Finalize(DXGI_FORMAT format);

		static constexpr int _k_swapChainBuffers = 3;

		DX12::RendererCommand* _linkedRendererCommand = nullptr; // non-owning
		Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
		HWND _window = NULL;
		mutable unsigned int _currentBackBufferIndex = 0;
		D3D12_VIEWPORT _viewport = {};
		D3D12_RECT _scissorRect = {};
		unsigned int _presentFlags = 0;
		unsigned int _allowTearing = 0;
		DX12::RenderTargetData _renderTargetData[DX12::CC::k_frameBuffers];
	};
}