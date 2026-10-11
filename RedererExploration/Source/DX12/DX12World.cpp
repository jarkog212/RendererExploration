#include "DX12World.h"
#include "DX12Mesh.h"

#include <unordered_map>

namespace
{
	static const int k_batchReserveAmount = 64;

	std::vector<std::weak_ptr<DX12::Meshes::Mesh>> g_allWorldMeshes;
	std::unordered_map<DX12::Materials::Type, std::vector<std::weak_ptr<DX12::Meshes::Mesh>>> g_materialMeshMap;
	std::vector<std::weak_ptr<DX12::Materials::Material>> g_allMaterials;

	std::vector<std::unique_ptr<DX12::World::Geometry>> g_worldGeometries;
	std::vector<std::unique_ptr<DX12::World::ComplexGeometry>> g_worldComplexGeometries;
	std::vector<std::unique_ptr<DX12::World::LODGeometry>> g_worldLODGeometries;
}

DX12::World::Geometry::Geometry(const std::shared_ptr<DX12::Meshes::Mesh>& mesh, const std::shared_ptr<DX12::Materials::Material>& material)
{
	CRITICAL_ASSERT(mesh, "Mesh is not valid.");
	CRITICAL_ASSERT(material, "Material is not valid.");

	_mesh = mesh;
	_material = material;
	_worldMatrixAllocation = std::make_unique<DX12::Meshes::WorldMatrixBufferAllocation>(_mesh->ReserveTransformMatrix());
}

DX12::World::Geometry::Geometry(const wchar_t* meshName, DX12::Materials::Type materialType)
{
	CRITICAL_ASSERT(meshName, "Mesh name is not valid.");

	_mesh = DX12::MeshSpace::GetOrLoad(meshName, nullptr, nullptr);
	_material = DX12::MaterialSpace::GetOrLoad(materialType);
	_worldMatrixAllocation = std::make_unique<DX12::Meshes::WorldMatrixBufferAllocation>(_mesh->ReserveTransformMatrix());
}

DirectX::XMMATRIX* DX12::World::Geometry::GetTargetMatrix()
{
	return !_worldMatrixAllocation ? nullptr : &(_worldMatrixAllocation->GetTransformMatrix());
}

DX12::World::ComplexGeometry::ComplexGeometry(std::vector<DX12::World::Geometry>&& subGeometries)
{
	_subGeometries.swap(subGeometries);
	for (auto& it : _subGeometries)
		static_cast<DX12::Attributes::Transform*>(&it)->AssignParent(static_cast<DX12::Attributes::Transform&>(*this));
}

DX12::World::LODGeometry::LODGeometry(std::vector<DX12::World::ComplexGeometry>&& complexGeometryLods)
{
	_complexGeometryLods.swap(complexGeometryLods);
	for (auto& it : _complexGeometryLods)
		static_cast<DX12::Attributes::Transform*>(&it)->AssignParent(static_cast<DX12::Attributes::Transform&>(*this));
}

const std::vector<std::weak_ptr<DX12::Materials::Material>>& DX12::WorldSpace::GetAllMaterials()
{
	return ::g_allMaterials;
}

const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>* DX12::WorldSpace::GetMeshesOfMaterial(const DX12::Materials::Type material)
{
	auto it = ::g_materialMeshMap.find(material);
	return it != ::g_materialMeshMap.end() ? nullptr : &it->second;
}

const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>& DX12::WorldSpace::GetAllMeshes()
{
	return ::g_allWorldMeshes;
}

void DX12::WorldSpace::WarmUp(const std::vector<DX12::Materials::Type>& materialList, const std::vector<std::string>& meshes)
{
}

void DX12::WorldSpace::ReleaseAll()
{

}


