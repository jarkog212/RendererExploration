#include "DX12World.h"
#include "DX12Mesh.h"

#include <map>
#include <unordered_map>

namespace
{
	static const int k_initialBatchReserveAmount = 64;

	template<typename T>
	struct WeakPtrLess {
		bool operator()(const std::weak_ptr<T>& a, const std::weak_ptr<T>& b) const {
			return a.owner_before(b);
		}
	};

	std::mutex g_meshTemplatesMutex;
	std::vector<std::weak_ptr<DX12::Meshes::Mesh>> g_allWorldMeshTemplates;
	std::map<std::weak_ptr<DX12::Meshes::Mesh>, size_t, WeakPtrLess<DX12::Meshes::Mesh>> g_meshTemplatePresence;

	std::mutex g_materialMeshTemplateMutex;
	std::unordered_map<DX12::Materials::Type, std::vector<std::weak_ptr<DX12::Meshes::Mesh>>> g_materialMeshTemplateMap;

	std::mutex g_materialMutex;
	std::vector<std::weak_ptr<DX12::Materials::Material>> g_allMaterials;
	std::map<std::weak_ptr<DX12::Materials::Material>, size_t, WeakPtrLess<DX12::Materials::Material>> g_materialPresence;

	std::mutex g_geometriesMutex;
	std::vector<std::unique_ptr<DX12::World::Geometry>> g_worldGeometries;
	std::unordered_map<DX12::World::Geometry*, size_t> g_worldGeometryPresence;

	std::mutex g_complexGeometriesMutex;
	std::vector<std::unique_ptr<DX12::World::ComplexGeometry>> g_worldComplexGeometries;
	std::unordered_map<DX12::World::ComplexGeometry*, size_t> g_worldComplexGeometryPresence;

	std::mutex g_lodGeometriesMutex;
	std::vector<std::unique_ptr<DX12::World::LODGeometry>> g_worldLODGeometries;
	std::unordered_map<DX12::World::LODGeometry*, size_t> g_worldLODGeometryPresence;

	void SafeAddGeometry(std::unique_ptr<DX12::World::Geometry>&& geometry)
	{
		std::scoped_lock lock(::g_geometriesMutex);

		if (::g_worldGeometries.capacity() == 0)
			::g_worldGeometries.reserve(::k_initialBatchReserveAmount);
		
		::g_worldGeometryPresence[geometry.get()] = ::g_worldGeometries.size();
		::g_worldGeometries.push_back(std::move(geometry));
	}

	void SafeRemoveGeometry(DX12::World::Geometry*& geometry)
	{
		std::unique_ptr<DX12::World::Geometry> threadSafeLifeline;

		{
			std::scoped_lock lock(::g_geometriesMutex);

			CRITICAL_ASSERT(::g_worldGeometryPresence.contains(geometry), "Trying to remove geometry that has not been added.");

			const size_t index = ::g_worldGeometryPresence[geometry];
			const size_t last = ::g_worldGeometries.size() - 1;
			::g_worldGeometryPresence.erase(geometry);

			std::swap(::g_worldGeometries[index], ::g_worldGeometries[last]);
			std::swap(::g_worldGeometries[last], threadSafeLifeline);
			::g_worldGeometries.pop_back();
			geometry = nullptr;

			if (::g_worldGeometries.size() == 0 || index >= last)
				return;

			::g_worldGeometryPresence[::g_worldGeometries[index].get()] = index;
		}

		// destructor of held lifeline is now called after scope lock
	}

	void SafeAddGeometry(std::unique_ptr<DX12::World::ComplexGeometry>&& complexGeometry)
	{
		std::scoped_lock lock(::g_complexGeometriesMutex);

		if (::g_worldComplexGeometries.capacity() == 0)
			::g_worldComplexGeometries.reserve(::k_initialBatchReserveAmount);

		::g_worldComplexGeometryPresence[complexGeometry.get()] = ::g_worldComplexGeometries.size();
		::g_worldComplexGeometries.push_back(std::move(complexGeometry));
	}

	void SafeRemoveGeometry(DX12::World::ComplexGeometry*& complexGeometry)
	{
		std::unique_ptr<DX12::World::ComplexGeometry> threadSafeLifeline;

		{
			std::scoped_lock lock(::g_complexGeometriesMutex);

			CRITICAL_ASSERT(::g_worldComplexGeometryPresence.contains(complexGeometry), "Trying to remove complex geometry that has not been added.");

			const size_t index = ::g_worldComplexGeometryPresence[complexGeometry];
			const size_t last = ::g_worldComplexGeometries.size() - 1;
			::g_worldComplexGeometryPresence.erase(complexGeometry);

			std::swap(::g_worldComplexGeometries[index], ::g_worldComplexGeometries[last]);
			std::swap(::g_worldComplexGeometries[last], threadSafeLifeline);
			::g_worldComplexGeometries.pop_back();
			complexGeometry = nullptr;

			if (::g_worldComplexGeometries.size() == 0 || index >= last)
				return;

			::g_worldComplexGeometryPresence[::g_worldComplexGeometries[index].get()] = index;
		}

		// destructor of held lifeline is now called after scope lock
	}

	void SafeAddGeometry(std::unique_ptr<DX12::World::LODGeometry>&& lodGeometry)
	{
		std::scoped_lock lock(::g_worldLODGeometries);

		if (::g_worldLODGeometries.capacity() == 0)
			::g_worldLODGeometries.reserve(::k_initialBatchReserveAmount);

		::g_worldLODGeometryPresence[lodGeometry.get()] = ::g_worldLODGeometries.size();
		::g_worldLODGeometries.push_back(std::move(lodGeometry));
	}

	void SafeRemoveGeometry(DX12::World::LODGeometry*& lodGeometry)
	{
		std::unique_ptr<DX12::World::LODGeometry> threadSafeLifeline;

		{
			std::scoped_lock lock(::g_lodGeometriesMutex);

			CRITICAL_ASSERT(::g_worldLODGeometryPresence.contains(lodGeometry), "Trying to remove lod geometry that has not been added.");

			const size_t index = ::g_worldLODGeometryPresence[lodGeometry];
			const size_t last = ::g_worldLODGeometries.size() - 1;
			::g_worldLODGeometryPresence.erase(lodGeometry);

			std::swap(::g_worldLODGeometries[index], ::g_worldLODGeometries[last]);
			std::swap(::g_worldLODGeometries[last], threadSafeLifeline);
			::g_worldLODGeometries.pop_back();
			lodGeometry = nullptr;

			if (::g_worldLODGeometries.size() == 0 || index >= last)
				return;

			::g_worldLODGeometryPresence[::g_worldLODGeometries[index].get()] = index;
		}

		// destructor of held lifeline is now called after scope lock
	}

	void SafeAddUniqueMeshTemplateToAll(const std::weak_ptr<DX12::Meshes::Mesh>& weakMesh)
	{
		std::scoped_lock lock(::g_meshTemplatesMutex);

		if (::g_meshTemplatePresence.contains(weakMesh))
			return;

		if (::g_allWorldMeshTemplates.capacity() == ::g_allWorldMeshTemplates.size())
			::g_allWorldMeshTemplates.reserve(::g_allWorldMeshTemplates.size() + ::k_initialBatchReserveAmount);

		::g_allWorldMeshTemplates.push_back(weakMesh);
		::g_meshTemplatePresence.emplace(weakMesh, ::g_allWorldMeshTemplates.size() - 1);
	}

	void SafeAddUniqueMaterialToAll(const std::weak_ptr<DX12::Materials::Material>& weakMaterial)
	{
		std::scoped_lock lock(::g_materialMutex);

		if (::g_materialPresence.contains(weakMaterial))
			return;

		if (::g_allMaterials.capacity() == ::g_allMaterials.size())
			::g_allMaterials.reserve(::g_allMaterials.size() + ::k_initialBatchReserveAmount);

		::g_allMaterials.push_back(weakMaterial);
		::g_materialPresence.emplace(weakMaterial, ::g_allWorldMeshTemplates.size() - 1);
	}

	void SafeRegisterMaterialMeshPair(DX12::Materials::Type type, const std::weak_ptr<DX12::Meshes::Mesh>& weakMesh)
	{
		std::scoped_lock lock(::g_materialMeshTemplateMutex);

		if (const auto& meshTemplatesIt = ::g_materialMeshTemplateMap.find(type); meshTemplatesIt != ::g_materialMeshTemplateMap.end())
		{
			auto& meshTemplates = meshTemplatesIt->second;
			for (const auto& meshTemplate : meshTemplates)
				if (meshTemplate.owner_before(weakMesh))
					return;

			//weak mesh should not have expired, and even if the weakness if the container implies unsafety

			if (meshTemplates.capacity() == meshTemplates.size())
				meshTemplates.reserve(meshTemplates.size() + ::k_initialBatchReserveAmount);

			meshTemplates.push_back(weakMesh);
			return;
		}

		std::pair<DX12::Materials::Type, std::vector<std::weak_ptr<DX12::Meshes::Mesh>>> newEntry;
		newEntry.first = type;
		newEntry.second.reserve(::k_initialBatchReserveAmount);
		newEntry.second.push_back(weakMesh);

		::g_materialMeshTemplateMap.insert(std::move(newEntry));
	}

	void SafeRegisterGeometryParts(const DX12::World::Geometry& geometry)
	{
		std::weak_ptr<DX12::Materials::Material> weakMaterial = geometry.GetWeakMaterial();

		::SafeAddUniqueMaterialToAll(weakMaterial);

		std::weak_ptr<DX12::Meshes::Mesh> weakMesh = geometry.GetWeakMesh();

		::SafeAddUniqueMeshTemplateToAll(weakMesh);

		DX12::Materials::Type materialType = geometry.GetMaterialType();

		::SafeRegisterMaterialMeshPair(materialType, weakMesh);
	}

	void SafeRegisterGeometryParts(const DX12::World::ComplexGeometry& complexGeometry)
	{
		const std::vector<DX12::World::Geometry>& simpleGeometries = complexGeometry.GetSimpleGeometries();
		for (const DX12::World::Geometry& geometry : simpleGeometries)
			::SafeRegisterGeometryParts(geometry);
	}
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

const DX12::World::ComplexGeometry* DX12::World::LODGeometry::GetGeometry(size_t index) const
{
	if (_complexGeometryLods.size() == 0)
		return nullptr;

	return &(_complexGeometryLods[std::min(index, _complexGeometryLods.size() - 1)]);
}

const std::vector<std::weak_ptr<DX12::Materials::Material>>& DX12::WorldSpace::GetAllMaterials()
{
	return ::g_allMaterials;
}

const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>* DX12::WorldSpace::GetMeshTemplatesOfMaterial(const DX12::Materials::Type material)
{
	const auto& meshTemplates = ::g_materialMeshTemplateMap.find(material);
	return meshTemplates != ::g_materialMeshTemplateMap.end() ? nullptr : &meshTemplates->second;
}

const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>& DX12::WorldSpace::GetAllMeshTemplates()
{
	return ::g_allWorldMeshTemplates;
}

void DX12::WorldSpace::WarmUp(const std::vector<DX12::Materials::Type>& materialList, const std::vector<std::string>& meshes)
{
	::g_allWorldMeshTemplates.reserve(::k_initialBatchReserveAmount);
	::g_allMaterials.reserve(::k_initialBatchReserveAmount);
	::g_worldGeometries.reserve(::k_initialBatchReserveAmount);
	::g_worldComplexGeometries.reserve(::k_initialBatchReserveAmount);
	::g_worldLODGeometries.reserve(::k_initialBatchReserveAmount);
}	

void DX12::WorldSpace::ReleaseAll()
{
	::g_allWorldMeshTemplates.clear();
	::g_meshTemplatePresence.clear();
	::g_materialMeshTemplateMap.clear();
	::g_allMaterials.clear();
	::g_worldGeometries.clear();
	::g_worldComplexGeometries.clear();
	::g_worldLODGeometries.clear();
}

DX12::World::Geometry* DX12::WorldSpace::AddObject(DX12::World::Geometry&& object)
{
	std::unique_ptr<DX12::World::Geometry> original = std::make_unique<DX12::World::Geometry>(std::move(object));
	DX12::World::Geometry* raw = original.get();

	::SafeAddGeometry(std::move(original));

	::SafeRegisterGeometryParts(*raw);

	return raw;
}

DX12::World::ComplexGeometry* DX12::WorldSpace::AddObject(DX12::World::ComplexGeometry&& object)
{
	std::unique_ptr<DX12::World::ComplexGeometry> original = std::make_unique<DX12::World::ComplexGeometry>(std::move(object));
	DX12::World::ComplexGeometry* raw = original.get();

	::SafeAddGeometry(std::move(original));

	::SafeRegisterGeometryParts(*raw);

	return raw;
}

DX12::World::LODGeometry* DX12::WorldSpace::AddObject(DX12::World::LODGeometry&& object)
{
	std::unique_ptr<DX12::World::LODGeometry> original = std::make_unique<DX12::World::LODGeometry>(std::move(object));
	DX12::World::LODGeometry* raw = original.get();

	::SafeAddGeometry(std::move(original));

	// Adds the LOD @ 0
	::SafeRegisterGeometryParts(*raw->GetGeometry(0));
}

void DX12::WorldSpace::RemoveObject(DX12::World::Geometry*& object)
{
	// TODO unregister parts

	::SafeRemoveGeometry(object);
}

void DX12::WorldSpace::RemoveObject(DX12::World::ComplexGeometry*& object)
{
	// TODO unregister parts

	::SafeRemoveGeometry(object);
}

void DX12::WorldSpace::RemoveObject(DX12::World::LODGeometry*& object)
{
	// TODO unregister parts

	::SafeRemoveGeometry(object);
}



