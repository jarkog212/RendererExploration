#pragma once

#include "DX12Resources.h"

#include <unordered_map>

namespace DX12::World
{
	std::vector<std::weak_ptr<DX12::Materials::Material>>& GetAllMaterials();

	std::vector<std::weak_ptr<DX12::Meshes::Mesh>>& GetMesheshOfMaterial(const DX12::Materials::Type material);

	std::vector<std::weak_ptr<DX12::Meshes::Mesh>>& GetAllMeshes();
}