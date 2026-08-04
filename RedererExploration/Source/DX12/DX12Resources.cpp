#include "DX12Resources.h"
#include "DX12Renderer.h" 

DX12::DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int capacity, bool isShaderVisible) : _type(type)
{
	if (capacity == 0
		|| capacity > D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2
		|| (_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE))
	{
		DISPLAY_ERROR(L"Bad capacity for a heap.");
		assert(false);
	}

	isShaderVisible &= (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	{
		std::lock_guard lock(_mutex);

		Release();
		ID3D12Device8* device = DX12::GetRendererDevice();

		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NumDescriptors = capacity;
		desc.Type = _type;
		desc.NodeMask = 0;

		HRESULT_CHECKED_FALLBACK(
			device->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), (void**)&_heap),
			assert(false),
			"Failed to create a descriptor heap.");

		_freeHandles = std::make_unique<unsigned int[]>(capacity);
		for (unsigned int i = 0; i < capacity; i++)
			_freeHandles[i] = i;

		_capacity = capacity;
		_size = 0;
		_descriptorSize = device->GetDescriptorHandleIncrementSize(_type);
		_mainHandles._cpu = _heap->GetCPUDescriptorHandleForHeapStart();

		if (isShaderVisible)
			_mainHandles._gpu = _heap->GetGPUDescriptorHandleForHeapStart();
	}
}

void DX12::DescriptorHeap::Release()
{
	if (_size > 0)
	{
		DISPLAY_ERROR(L"Trying to free a non empty heap.");
		assert(false);
	}

	Utils::COM::SafeDeferredRelease(_heap);
}

DX12::DescriptorHeap::DescriptorHandle DX12::DescriptorHeap::Allocate()
{
	std::lock_guard lock(_mutex);

	if (!_heap)
	{
		DISPLAY_ERROR(L"Trying to allocate descriptor on invalid heap.");
		assert(false);
	}

	if (_size >= _capacity)
	{
		DISPLAY_ERROR(L"Trying to allocate descriptor to full heap.")
		assert(false);
	}

	const int index = _freeHandles[_size];
	int offset = index * _descriptorSize;
	_size++;

	DX12::DescriptorHeap::DescriptorHandle out;
	out._cpu.ptr = _mainHandles._cpu.ptr + offset;

	if (IsShaderVisible())
		out._gpu.ptr = _mainHandles._gpu.ptr + offset;

#ifdef _DEBUG
	{
		out._owner = this;
		out._index = index;
	}
#endif

	return out;
}

void DX12::DescriptorHeap::Free(DescriptorHandle& handle)
{
	if (!handle.IsValid())
		return;
	
	{
		std::lock_guard lock(_mutex);

		if (!_heap || _size == 0)
		{
			DISPLAY_ERROR(L"Called Free on an invalid or empty descriptor heap.");
			assert(false);
		}

		const bool isHandleManagedByUs =
			handle._cpu.ptr >= _mainHandles._cpu.ptr
			|| handle._cpu.ptr < (_mainHandles._cpu.ptr + _descriptorSize * _capacity);

#ifdef _DEBUG
		if (handle._owner != this || !isHandleManagedByUs)
#elif
		if (!isHandleManagedByUs)
#endif
		{
			DISPLAY_ERROR(L"Called Free for handle not managed by this heap.");
			assert(false);
		}

		if ((handle._cpu.ptr - _mainHandles._cpu.ptr) % _descriptorSize == 0)
		{
			DISPLAY_ERROR(L"Called Free for handle whose addresses are not aligned correctly.");
			assert(false);
		}

		unsigned int index = (handle._cpu.ptr - _mainHandles._cpu.ptr) / _descriptorSize;

#ifdef _DEBUG
		{
			if (index == handle._index)
			{
				DISPLAY_ERROR(L"Called Free for handle whose index is not matching what it got initialised with.");
				assert(false);
			}
		}
#endif

		const int currentFrame = DX12::GetMainRenderCommand()->GetCurrentFrame();
		_deferredResourceFreeing[currentFrame].push_back(index);
		DX12::SetPendingResourceFreeingFlag();

		handle = {};
	}
}

void DX12::DescriptorHeap::ProcessPendingResourceFreeing(const int currentFrame)
{
	std::lock_guard lock(_mutex);

	auto cleaner = [&](const unsigned int frame)
		{
			std::vector<unsigned int>& indeces = _deferredResourceFreeing[frame];
			if (indeces.empty())
				return;

			for (auto it : indeces)
			{
				--_size;
				_freeHandles[_size] = it;
			}

			indeces.clear();
		};

	if (currentFrame == k_allPending)
	{
		for (int i = 0; i < k_frameBuffers; i++)
			cleaner(i);

		return;
	}

	cleaner(currentFrame);
}

DX12::RenderSurface::RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue1* commandQueue) 
	: _window(window)
{
	RECT windowDimensions = GetWindowDimensions();
	const int width = windowDimensions.right - windowDimensions.left;
	const int height = windowDimensions.bottom - windowDimensions.top;

	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount = DX12::k_frameBuffers;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = 0;
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

	IDXGIFactory7* factory = DX12::GetDxgiFactory();
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
		swapChain->QueryInterface<IDXGISwapChain4>(_swapChain.GetAddressOf()),
		assert(false),
		"Could not get newer swap chain interface.");

	Utils::COM::SafeRelease(swapChain);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	for (int i = 0; i < DX12::k_frameBuffers; i++)
		_renderTargetData[i]._handle = DX12::GetRTVHeap()->Allocate();

	Finalize();
}

void DX12::RenderSurface::Present() const
{
	if (!_swapChain)
	{
		DISPLAY_ERROR(L"Trying to present an invalid swapchain.");
		assert(false);
	}

	_swapChain->Present(0, 0);
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

RECT DX12::RenderSurface::GetWindowDimensions() const
{
	RECT rect;
	if (!GetWindowRect(_window, &rect))
		DISPLAY_ERROR(L"Error getting window dimensions.");

	return rect;
}

void DX12::RenderSurface::Release()
{
	for (int i = 0; i < DX12::k_frameBuffers; i++)
		_renderTargetData[i].Release();

	_swapChain.Reset();
}

void DX12::RenderSurface::Finalize()
{
	for (int i = 0; i < DX12::k_frameBuffers; i++)
	{
		RenderTargetData& data = _renderTargetData[i];
		HRESULT_CHECKED(
			_swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)data._resource.GetAddressOf()),
			"Could not get a hold of the swap chain internal buffer.");

		D3D12_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = DX12::k_defaultRenderTargetFormat;
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		DX12::GetRendererDevice()->CreateRenderTargetView(data._resource.Get(), &desc, data._handle._cpu);
	}

	DXGI_SWAP_CHAIN_DESC desc;
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

void DX12::RenderSurface::RenderTargetData::Release()
{
	_resource.Reset();
	DX12::GetRTVHeap()->Free(_handle);
}
