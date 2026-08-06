#pragma once

#include "DX12Core.h"

namespace DX12::CC::Desc
{
	constexpr D3D12_HEAP_PROPERTIES k_defaultHeapProperties{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};
}