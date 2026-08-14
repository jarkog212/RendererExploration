#include "DX12RenderSurface.h"
#include "DX12Core.h"
#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

DX12::RenderSurface::RenderSurface(const HWND window, DXGI_FORMAT format, DX12::RendererCommand& rendererCommand)
	: _window(window)
{
	IDXGIFactory7* factory = DX12::GetDxgiFactory();

	HRESULT_CHECK_CRITICAL(
		factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allowTearing, sizeof(_allowTearing)),
		"Could not test for tearing support.");

	if ((bool)_allowTearing)
		_presentFlags = DXGI_PRESENT_ALLOW_TEARING;

	ResetSwapChain(rendererCommand, format);

	HRESULT_CHECK_FALLBACK(
		factory->MakeWindowAssociation(_window, DXGI_MWA_NO_ALT_ENTER), ,
		assert(false),
		"Could not restirct fullscreen on swapchain.");

	Finalize(format);
}

DX12::RenderSurface::~RenderSurface()
{
	// Stall out the GPU
	_linkedRendererCommand->Flush();
}

DX12::RenderSurface::RenderSurface(RenderSurface&& other) noexcept :
	_linkedRendererCommand(std::exchange(other._linkedRendererCommand, nullptr)),
	_swapChain(std::move(other._swapChain)),
	_window(std::exchange(other._window, HWND(NULL))),
	_currentBackBufferIndex(std::exchange(other._currentBackBufferIndex, 0)),
	_viewport(std::exchange(other._viewport, D3D12_VIEWPORT())),
	_scissorRect(std::exchange(other._scissorRect, D3D12_RECT())),
	_presentFlags(std::exchange(other._presentFlags, 0)),
	_allowTearing(std::exchange(other._allowTearing, 0))
{
	for (int i = 0; i < _k_swapChainBuffers; i++)
		_renderTargetData[i] = std::move(other._renderTargetData[i]);
}

DX12::RenderSurface& DX12::RenderSurface::operator=(RenderSurface&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving RenderSurface to itself.");

	std::swap(_linkedRendererCommand, other._linkedRendererCommand);
	std::swap(_swapChain, other._swapChain);
	std::swap(_window, other._window);
	std::swap(_currentBackBufferIndex, other._currentBackBufferIndex);
	std::swap(_viewport, other._viewport);
	std::swap(_scissorRect, other._scissorRect);
	std::swap(_presentFlags, other._presentFlags);
	std::swap(_allowTearing, other._allowTearing);

	for (int i = 0; i < _k_swapChainBuffers; i++)
		std::swap(_renderTargetData[i], other._renderTargetData[i]);

	return *this;
}

void DX12::RenderSurface::Resize()
{
	if (_linkedRendererCommand)
		_linkedRendererCommand->Flush();

	// body
}

void DX12::RenderSurface::PreRender()
{
	CRITICAL_ASSERT(_linkedRendererCommand, "Cannot prepare render without valid command executor.");

	_linkedRendererCommand->PreRender();
}

void DX12::RenderSurface::PostRender()
{
	CRITICAL_ASSERT(_linkedRendererCommand, "Cannot finish render without valid command executor.");

	_linkedRendererCommand->PostRender();
}

void DX12::RenderSurface::Present() const
{
	CRITICAL_ASSERT(_swapChain, "Trying to present an invalid swap chain.");

	_swapChain->Present(0, _presentFlags);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

void DX12::RenderSurface::UpdateViewport()
{
	ID3D12GraphicsCommandList6* commandList = _linkedRendererCommand->GetCommandList();
	commandList->RSSetViewports(1, &_viewport);
	commandList->RSSetScissorRects(1, &_scissorRect);
}

void DX12::RenderSurface::ResetSwapChain(DX12::RendererCommand& rendererCommand, DXGI_FORMAT format)
{
	if (_linkedRendererCommand)
		_linkedRendererCommand->Flush();

	_linkedRendererCommand = &rendererCommand;

	RECT windowDimensions = GetWindowDimensions();
	const int width = windowDimensions.right - windowDimensions.left;
	const int height = windowDimensions.bottom - windowDimensions.top;

	DXGI_SWAP_CHAIN_DESC1 desc = DX12::CC::Desc::SwapChain::k_descTemplate;
	desc.BufferCount = _k_swapChainBuffers;
	desc.Flags = (bool)_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ?
		DXGI_FORMAT_R8G8B8A8_UNORM :
		format;
	desc.Height = height;
	desc.Width = width;

	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain = nullptr;
		HRESULT_CHECK_FALLBACK(
			DX12::GetDxgiFactory()->CreateSwapChainForHwnd(
				_linkedRendererCommand->GetCommandQueue(), _window, &desc, nullptr, nullptr, &swapChain),,
			assert(false),
			"Could not create a swap chain.");

		HRESULT_CHECK_FALLBACK(
			swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain)),,
			assert(false),
			"Could not get newer swap chain interface.");
	}

	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	for (int i = 0; i < _k_swapChainBuffers; i++)
	{
		_renderTargetData[i].Release();
		_renderTargetData[i]._rtvHandle = DX12::GetRTVHeap()->Allocate();
	}

	Finalize(format);
}

RECT DX12::RenderSurface::GetWindowDimensions() const
{
	RECT rect;
	FALLBACK_ASSERT(GetWindowRect(_window, &rect), ,"Error getting window dimensions.")
	return rect;
}

void DX12::RenderSurface::Finalize(DXGI_FORMAT format)
{
	for (int i = 0; i < _k_swapChainBuffers; i++)
	{
		RenderTargetData& data = _renderTargetData[i];
		HRESULT_CHECK_CRITICAL(
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&data._resource)),
			"Could not get a hold of the swap chain internal buffer.");

		D3D12_RENDER_TARGET_VIEW_DESC desc = DX12::CC::Desc::RTView::k_descTemplate;
		desc.Format = format;

		DX12::GetRendererDevice()->CreateRenderTargetView(data._resource.Get(), &desc, data._rtvHandle._cpu);
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	HRESULT_CHECK_CRITICAL(_swapChain->GetDesc(&desc), "Could not get swap chain descriptor object.");

	const unsigned int height = desc.BufferDesc.Height;
	const unsigned int width = desc.BufferDesc.Width;

	RECT windowDimensions = GetWindowDimensions();
	int winWidth = windowDimensions.right - windowDimensions.left;
	int winHeight = windowDimensions.bottom - windowDimensions.top;

	CRITICAL_ASSERT(
		height == winHeight && width == winWidth,
		"Dimensions of the swap chain buffers and the window do not match.");

	_viewport.TopLeftX = 0.f;
	_viewport.TopLeftY = 0.f;
	_viewport.Width = (float)width;
	_viewport.Height = (float)height;
	_viewport.MinDepth = 0.f;
	_viewport.MaxDepth = 1.f;

	_scissorRect = { 0,0, (long)width, (long)height };
}