#include "DX12DescriptorHeap.h"
#include "DX12Renderer.h"

DX12::DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int capacity, bool isShaderVisible) : _type(type)
{
	CRITICAL_ASSERT(
		capacity != 0
		&& capacity <= D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2
		&& (_type != D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER || capacity <= D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE),
		"Bad capacity for a heap.");

	isShaderVisible &= (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	{
		std::lock_guard lock(_mutex);

		Release();
		ID3D12Device8* device = DX12::GetRendererDevice();

		D3D12_DESCRIPTOR_HEAP_DESC desc{};
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
	CRITICAL_ASSERT(_size <= 0, "Trying to free a non empty heap.");

	Utils::COM::SafeDeferredRenderThreadRelease(_heap);
}

DX12::DescriptorHandle DX12::DescriptorHeap::Allocate()
{
	std::lock_guard lock(_mutex);

	CRITICAL_ASSERT(_heap, "Trying to allocate descriptor on invalid heap.");
	CRITICAL_ASSERT(_size < _capacity, "Trying to allocate descriptor to full heap.");

	const int index = _freeHandles[_size];
	int offset = index * _descriptorSize;
	_size++;

	DX12::DescriptorHandle out;
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

		CRITICAL_ASSERT(_heap && _size > 0, "Called Free on an invalid or empty descriptor heap.");

		const bool isHandleManagedByUs =
			handle._cpu.ptr >= _mainHandles._cpu.ptr
			|| handle._cpu.ptr < (_mainHandles._cpu.ptr + _descriptorSize * _capacity);

#ifdef _DEBUG
		CRITICAL_ASSERT(handle._owner == this && isHandleManagedByUs, "Called Free for handle not managed by this heap.");
#elif
		CRITICAL_ASSERT(isHandleManagedByUs, "Called Free for handle not managed by this heap.");
#endif
		CRITICAL_ASSERT(
			(handle._cpu.ptr - _mainHandles._cpu.ptr) % _descriptorSize == 0,
			"Called Free for handle whose addresses are not aligned correctly.");

		unsigned int index = (handle._cpu.ptr - _mainHandles._cpu.ptr) / _descriptorSize;

#ifdef _DEBUG
		CRITICAL_ASSERT(index == handle._index, "Called Free for handle whose index is not matching what it got initialised with.");
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

	if (currentFrame == DX12::CC::k_forAllFrames)
	{
		for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
			cleaner(i);

		return;
	}

	cleaner(currentFrame);
}