#include "DX12RenderSurface.h"
#include "DX12Core.h"
#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

DX12::RenderSurface::RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue* commandQueue)
	: _window(window)
{
	IDXGIFactory7* factory = DX12::GetDxgiFactory();

	HRESULT_CHECK_CRITICAL(
		factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allowTearing, sizeof(_allowTearing)),
		"Could not test for tearing support.");

	if ((bool)_allowTearing)
		_presentFlags = DXGI_PRESENT_ALLOW_TEARING;

	RECT windowDimensions = GetWindowDimensions();
	const int width = windowDimensions.right - windowDimensions.left;
	const int height = windowDimensions.bottom - windowDimensions.top;

	DXGI_SWAP_CHAIN_DESC1 desc = DX12::CC::Desc::k_defaultSwapChainTemplate;
	desc.BufferCount = _k_swapChainBuffers;
	desc.Flags = (bool)_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ?
		DXGI_FORMAT_R8G8B8A8_UNORM :
		format;
	desc.Height = height;
	desc.Width = width;

	IDXGISwapChain1* swapChain = nullptr;
	HRESULT_CHECK_FALLBACK(
		factory->CreateSwapChainForHwnd(commandQueue, _window, &desc, nullptr, nullptr, &swapChain),,
		assert(false),
		"Could not create a swap chain.");

	HRESULT_CHECK_FALLBACK(
		factory->MakeWindowAssociation(_window, DXGI_MWA_NO_ALT_ENTER),,
		assert(false),
		"Could not restirct fullscreen on swapchain.");

	HRESULT_CHECK_FALLBACK(
		swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain)),,
		assert(false),
		"Could not get newer swap chain interface.");

	Utils::COM::SafeRelease(swapChain);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	for (int i = 0; i < _k_swapChainBuffers; i++)
		_renderTargetData[i]._rtvHandle = DX12::GetRTVHeap()->Allocate();

	Finalize(format);
}

void DX12::RenderSurface::Resize()
{
	DX12::GetMainCommandExecutor()->Flush();

	// body
}

void DX12::RenderSurface::Present() const
{
	CRITICAL_ASSERT(_swapChain, "Trying to present an invalid swap chain.");

	_swapChain->Present(0, _presentFlags);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

RECT DX12::RenderSurface::GetWindowDimensions() const
{
	RECT rect;
	FALLBACK_ASSERT(GetWindowRect(_window, &rect), ,"Error getting window dimensions.")
	return rect;
}

DX12::RenderSurface& DX12::RenderSurface::MoveImpl(RenderSurface&& other)
{
	CRITICAL_ASSERT(this != &other, "Moving RenderSurface to itself.");

	_window = other._window;
	_currentBackBufferIndex = other._currentBackBufferIndex;
	_viewport = other._viewport;
	_scissorRect = other._scissorRect;
	_presentFlags = other._presentFlags;
	_allowTearing = other._allowTearing;

	for (int i = 0; i < _k_swapChainBuffers; i++)
		_renderTargetData[i] = std::move(other._renderTargetData[i]);

	// clean non-move specified data
	other._window = NULL;
	other._currentBackBufferIndex = 0;
	other._viewport = D3D12_VIEWPORT();
	other._scissorRect = D3D12_RECT();
	other._presentFlags = 0;
	other._allowTearing = 0;

	return *this;
}

void DX12::RenderSurface::Release()
{
	DX12::GetMainCommandExecutor()->Flush();

	for (int i = 0; i < _k_swapChainBuffers; i++)
		_renderTargetData[i].Release();

	_swapChain.Reset();
}

void DX12::RenderSurface::Finalize(DXGI_FORMAT format)
{
	for (int i = 0; i < _k_swapChainBuffers; i++)
	{
		RenderTargetData& data = _renderTargetData[i];
		HRESULT_CHECK_CRITICAL(
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&data._resource)),
			"Could not get a hold of the swap chain internal buffer.");

		D3D12_RENDER_TARGET_VIEW_DESC desc = DX12::CC::Desc::k_defaultRenderTargetViewTemplate;
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