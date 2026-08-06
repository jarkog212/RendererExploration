#include "DX12RenderSurface.h"
#include "DX12Core.h"
#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"

DX12::RenderSurface::RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue* commandQueue)
	: _window(window)
{
	IDXGIFactory7* factory = DX12::GetDxgiFactory();

	HRESULT_CHECKED(
		factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allowTearing, sizeof(_allowTearing)),
		"Could not test for tearing support.");

	if ((bool)_allowTearing)
		_presentFlags = DXGI_PRESENT_ALLOW_TEARING;

	RECT windowDimensions = GetWindowDimensions();
	const int width = windowDimensions.right - windowDimensions.left;
	const int height = windowDimensions.bottom - windowDimensions.top;

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount = _k_swapChainBuffers;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = (bool)_allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ?
		DXGI_FORMAT_R8G8B8A8_UNORM :
		format;
	desc.Height = height;
	desc.Width = width;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Stereo = false;

	IDXGISwapChain1* swapChain = nullptr;
	HRESULT_CHECKED_FALLBACK(
		factory->CreateSwapChainForHwnd(commandQueue, _window, &desc, nullptr, nullptr, &swapChain),
		assert(false),
		"Could not create a swap chain.");

	HRESULT_CHECKED_FALLBACK(
		factory->MakeWindowAssociation(_window, DXGI_MWA_NO_ALT_ENTER),
		assert(false),
		"Could not restirct fullscreen on swapchain.");

	HRESULT_CHECKED_FALLBACK(
		swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain)),
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
	DX12::GetMainRenderCommand()->Flush();

	// body
}

void DX12::RenderSurface::Present() const
{
	if (!_swapChain)
	{
		DISPLAY_ERROR(L"Trying to present an invalid swapchain.");
		assert(false);
	}

	_swapChain->Present(0, _presentFlags);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

RECT DX12::RenderSurface::GetWindowDimensions() const
{
	RECT rect;
	if (!GetWindowRect(_window, &rect))
		DISPLAY_ERROR(L"Error getting window dimensions.");

	return rect;
}

void DX12::RenderSurface::MoveImpl(RenderSurface&& other)
{
	assert(this != &other);

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
}

void DX12::RenderSurface::Release()
{
	DX12::GetMainRenderCommand()->Flush();

	for (int i = 0; i < _k_swapChainBuffers; i++)
		_renderTargetData[i].Release();

	_swapChain.Reset();
}

void DX12::RenderSurface::Finalize(DXGI_FORMAT format)
{
	for (int i = 0; i < _k_swapChainBuffers; i++)
	{
		RenderTargetData& data = _renderTargetData[i];
		HRESULT_CHECKED(
			_swapChain->GetBuffer(i, IID_PPV_ARGS(&data._resource)),
			"Could not get a hold of the swap chain internal buffer.");

		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = format;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		DX12::GetRendererDevice()->CreateRenderTargetView(data._resource.Get(), &desc, data._rtvHandle._cpu);
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	HRESULT_CHECKED(_swapChain->GetDesc(&desc), "Could not get swap chain descriptor object.");

	const unsigned int height = desc.BufferDesc.Height;
	const unsigned int width = desc.BufferDesc.Width;

	RECT windowDimensions = GetWindowDimensions();
	int winWidth = windowDimensions.right - windowDimensions.left;
	int winHeight = windowDimensions.bottom - windowDimensions.top;

	if (height != winHeight || width != winWidth)
	{
		DISPLAY_ERROR(L"Dimensions of the swap chain buffers and the window do not match.");
		assert(false);
	}

	_viewport.TopLeftX = 0.f;
	_viewport.TopLeftY = 0.f;
	_viewport.Width = (float)width;
	_viewport.Height = (float)height;
	_viewport.MinDepth = 0.f;
	_viewport.MaxDepth = 1.f;

	_scissorRect = { 0,0, (long)width, (long)height };
}