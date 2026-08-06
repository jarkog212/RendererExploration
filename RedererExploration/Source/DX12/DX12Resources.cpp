#include "DX12Resources.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

void DX12::DescriptorHandle::MoveImpl(DescriptorHandle&& other)
{
	_cpu = other._cpu;
	_gpu = other._gpu;

	other._cpu = { 0 };
	other._gpu = { 0 };

#ifdef _DEBUG
	{
		_owner = other._owner;
		_index = other._index;

		other._owner = nullptr;
		other._index = 0;
	}
#endif
}

void DX12::RenderTargetData::Release()
{
	_resource.Reset();
	DX12::GetRTVHeap()->Free(_rtvHandle);
}

void DX12::RenderTargetData::MoveImpl(RenderTargetData&& other)
{
	assert(this != &other);

	_resource = std::move(other._resource);
	_rtvHandle = std::move(other._rtvHandle);
}






