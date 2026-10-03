#include "DX12Material.h"
#include "DX12GPass.h"
#include "DX12UploadRequest.h"

#include <unordered_map>
#include <memory>
#include <mutex>

namespace
{
	std::unordered_map<DX12::Materials::Type, std::weak_ptr<DX12::Materials::Material>> g_materials;

	std::mutex g_materialMutex;

	bool SafeReleaseMaterial(DX12::Materials::Type type)
	{
		std::lock_guard lock(::g_materialMutex);

		auto it = ::g_materials.find(type);
		if (it == ::g_materials.end())
			return false;

		CRITICAL_ASSERT(
			it->second.expired(),
			"Releasing material with legitimate references. %u",
			(unsigned int)type);

		::g_materials.erase(it);
		return true;
	}
}

std::shared_ptr<DX12::Materials::Material> DX12::MaterialSpace::GetOrLoad(DX12::Materials::Type type)
{
	std::lock_guard lock(::g_materialMutex);

	if (auto it = ::g_materials.find(type); it != ::g_materials.end() && !it->second.expired()) [[likely]]
		return it->second.lock();

#define MATERIAL_CASE(ENUM, TYPE) case ENUM: {\
	std::shared_ptr<DX12::Materials::Material> material = std::make_shared<##TYPE>();\
	::g_materials[type] = material;\
	return material; }

	switch (type)
	{
		MATERIAL_LIST_OP(MATERIAL_CASE)
	}

#undef MATERIAL_CASE
}

void DX12::MaterialSpace::Release() 
{
	std::lock_guard lock(::g_materialMutex);

	::g_materials.clear();
};

void DX12::MaterialSpace::WarmUp()
{
};

DX12::MaterialLib::BasicMaterial::BasicMaterial()
	: DX12::Materials::Material(DX12::Materials::Type::Basic)
{
	_pixelShader = DX12::ShaderSpace::GetOrLoad(GetPixelShaderSpec());
	_vertexShader = DX12::ShaderSpace::GetOrLoad(GetVertexShaderSpec());

	struct : public DX12::GPassSpace::PSOBase {
		PipelineStream::SubVertexShader _vs;
		PipelineStream::SubPixelShader _ps;
	}stream;
	stream._ps = _pixelShader->AsDX12Bytecode();
	stream._vs = _vertexShader->AsDX12Bytecode();

	_pipelineStateObject = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		_pipelineStateObject->SetName(L"Basic Material PSO."),
		"Could not name Basic Material PSO.");

	// HERE <setup the constant buffer and indeces to other stuff here as this is where the buffer is uploaded once>

	const unsigned int bufferSize = sizeof(_mappedConstantStructuredBuffer);
	_constStructuredBuffer = DX12::UploadDataToNewGPUBuffer(&_mappedConstantStructuredBuffer, bufferSize);
	_structuredConstantBufferSRVHandle = DX12::GetSRVHeap()->Allocate();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::BufferResource::k_StructuredSRVTemplate;
	srvDesc.Buffer.StructureByteStride = bufferSize;

	DX12::GetRendererDevice()->CreateShaderResourceView(_constStructuredBuffer, &srvDesc, _structuredConstantBufferSRVHandle._cpu);
}

DX12::MaterialLib::BasicMaterial::~BasicMaterial()
{
	DEBUG_DESTRUCTOR_BODY(BasicMaterial);
	::SafeReleaseMaterial(_type);
	Utils::SafeDeferredRenderThreadReleaseViaLambda(
		[
			constStructuredBuffer = _constStructuredBuffer,
			pso = _pipelineStateObject,
			handle = _structuredConstantBufferSRVHandle.MoveToRawHeap()
		]
		() mutable
		{
			Utils::COM::SafeRelease(constStructuredBuffer);
			Utils::COM::SafeRelease(pso);
			delete handle;
		}
);
}

ID3D12PipelineState* DX12::MaterialLib::BasicMaterial::GetPSO() const
{
	return _pipelineStateObject;
}

unsigned int DX12::MaterialLib::BasicMaterial::GetConstantBufferSRVIndex() const
{
	return _structuredConstantBufferSRVHandle._index;
}

void DX12::MaterialLib::BasicMaterial::AccummulateMaterialCommands(ID3D12GraphicsCommandList6* commandList)
{
	//<Custom command HERE>
}

const DX12::Shaders::Specs& DX12::MaterialLib::BasicMaterial::GetPixelShaderSpec()
{
	static const DX12::Shaders::Specs specs{
		L"", //_filename
		L"", //_function
		DX12::Shaders::Stage::Pixel,
		DX12::Shaders::GenericShader::Type::NotGeneric
	};

	return specs;
}

const DX12::Shaders::Specs& DX12::MaterialLib::BasicMaterial::GetVertexShaderSpec()
{
	static const DX12::Shaders::Specs specs{
		L"", //_filename
		L"", //_function
		DX12::Shaders::Stage::Vertex,
		DX12::Shaders::GenericShader::Type::NotGeneric
	};

	return specs;
}
