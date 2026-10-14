#pragma once

#include "DX12Resources.h"
#include "DX12Attributes.h"
#include "DX12Material.h"

namespace DX12
{
	namespace Meshes
	{
		class WorldMatrixBufferAllocation;
	}
}

namespace DX12::World
{

class Geometry : public DX12::Attributes::Holder<Geometry>,
	public DX12::Attributes::GPUMappedTransform
{
public:

	Geometry(const std::shared_ptr<DX12::Meshes::Mesh>& mesh, const std::shared_ptr<DX12::Materials::Material>& material);
	Geometry(const wchar_t* meshName, DX12::Materials::Type materialType);	
	DEBUG_DESTRUCTOR(Geometry);
	Geometry(Geometry&&) = default;
	Geometry& operator=(Geometry&&) = default;

	NO_COPY(Geometry);

	std::weak_ptr<DX12::Meshes::Mesh> GetWeakMesh() const{ return _mesh; }
	std::weak_ptr<DX12::Materials::Material> GetWeakMaterial() const { return _material; }
	DX12::Materials::Type GetMaterialType() const { return _material->GetType(); }

protected:

	// GPUMappedTransform
	DirectX::XMMATRIX* GetTargetMatrix() final override;
	// end GPUMappedTransform

	std::shared_ptr<DX12::Meshes::Mesh> _mesh;
	std::unique_ptr<DX12::Meshes::WorldMatrixBufferAllocation> _worldMatrixAllocation;
	std::shared_ptr<DX12::Materials::Material> _material;
};

class ComplexGeometry : public DX12::Attributes::Holder<ComplexGeometry>,
	public DX12::Attributes::Transform
{
public:

	ComplexGeometry(std::vector<DX12::World::Geometry>&& subGeometries);
	DEBUG_DESTRUCTOR(ComplexGeometry);
	ComplexGeometry(ComplexGeometry&&) = default;
	ComplexGeometry& operator=(ComplexGeometry&&) = default;

	NO_COPY(ComplexGeometry);

	const std::vector<DX12::World::Geometry>& GetSimpleGeometries() const { return _subGeometries; }

protected:

	std::vector<DX12::World::Geometry> _subGeometries;
};

class LODGeometry : public DX12::Attributes::Holder<LODGeometry>,
	public DX12::Attributes::Transform,
	public DX12::Attributes::IUpdatable
{
public:

	LODGeometry(std::vector<DX12::World::ComplexGeometry>&& complexGeometryLods);
	DEBUG_DESTRUCTOR(LODGeometry);
	LODGeometry(LODGeometry&&) = default;
	LODGeometry& operator=(LODGeometry&&) = default;

	NO_COPY(LODGeometry);

	const DX12::World::ComplexGeometry* GetGeometry(size_t index) const;

protected:

	std::vector<DX12::World::ComplexGeometry> _complexGeometryLods;

	//DX12::Attributes::IUpdatable
	void Update(float deltaTime) override {};
	// end DX12::Attributes::IUpdatable
};

}

namespace DX12::WorldSpace
{
	const std::vector<std::weak_ptr<DX12::Materials::Material>>& GetAllMaterials();
	const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>* GetMeshTemplatesOfMaterial(const DX12::Materials::Type material);
	const std::vector<std::weak_ptr<DX12::Meshes::Mesh>>& GetAllMeshTemplates();

	void WarmUp(const std::vector<DX12::Materials::Type>& materialList, const std::vector<std::string>& meshes);
	void ReleaseAll();

	DX12::World::Geometry* AddObject(DX12::World::Geometry&& object);
	DX12::World::ComplexGeometry* AddObject(DX12::World::ComplexGeometry&& object);
	DX12::World::LODGeometry* AddObject(DX12::World::LODGeometry&& object);
	void RemoveObject(DX12::World::Geometry*& object);
	void RemoveObject(DX12::World::ComplexGeometry*& object);
	void RemoveObject(DX12::World::LODGeometry*& object);
}