#include "DX12Material.h"

#include <unordered_map>
#include <memory>
#include <mutex>

namespace
{
	std::unordered_map<DX12::Materials::Type, std::unique_ptr<DX12::Materials::Material>> g_Materials;

	std::mutex g_mutex;
}

const DX12::Materials::Material& DX12::Materials::GetOrLoad(DX12::Materials::Type type)
{
	auto item = ::g_Materials.find(type);
	if (item != ::g_Materials.end())
		return *(item->second);

	std::scoped_lock lock(::g_mutex);

#define MATERIAL_CASE(ENUM, TYPE) case ENUM: ::g_Materials.emplace(ENUM, std::make_unique<##TYPE>()); break;

	switch (type)
	{
		MATERIAL_LIST_OP(MATERIAL_CASE)
	}

#undef MATERIAL_CASE
}

void DX12::Materials::Unload(DX12::Materials::Type type)
{
	std::scoped_lock lock(::g_mutex);

	auto item = ::g_Materials.find(type);
	if (item == ::g_Materials.end())
		return;

	::g_Materials.erase(item);
}

void DX12::Materials::Release() 
{
	::g_Materials.clear();
};

void DX12::Materials::WarmUp()
{
#define MATERIAL_WARMUP(ENUM, TYPE) ::g_Materials.emplace(ENUM, std::make_unique<##TYPE>());

	MATERIAL_LIST_OP(MATERIAL_WARMUP)

#undef MATERIAL_WARMUP
};
