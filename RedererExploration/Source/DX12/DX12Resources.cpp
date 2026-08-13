#include "DX12Resources.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

DX12::DescriptorHandle::~DescriptorHandle()
{
	if (_owner)
		_owner->Free(*this);
}

DX12::DescriptorHandle::DescriptorHandle(DescriptorHandle&& other) noexcept :
	_owner(std::exchange(other._owner, nullptr))
#ifdef _DEBUG
	,_index(std::exchange(other._index, 0))
#endif
{
	_cpu = std::exchange(other._cpu, { 0 });
	_gpu = std::exchange(other._gpu, { 0 });
}

DX12::DescriptorHandle& DX12::DescriptorHandle::operator=(DescriptorHandle&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Trying to move descriptor handle to itself.");

	std::swap(_cpu, other._cpu);
	std::swap(_gpu, other._gpu);
	std::swap(_owner, other._owner);

#ifdef _DEBUG
	{
		std::swap(_index, other._index);
	}
#endif

	return *this;
}

DX12::RenderTargetData::RenderTargetData(RenderTargetData&& other) noexcept :
	_resource(std::move(other._resource)),
	_rtvHandle(std::move(other._rtvHandle))
{
}

DX12::RenderTargetData& DX12::RenderTargetData::operator=(RenderTargetData&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving RenderTargetData to itself.");

	std::swap(_resource, other._resource);
	std::swap(_rtvHandle, other._rtvHandle);

	return *this;
}

void DX12::RootParameter::AsConstants(unsigned int numConstants, D3D12_SHADER_VISIBILITY visibility, unsigned int shaderRegister, unsigned int space)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	ShaderVisibility = visibility;
	Constants.ShaderRegister = shaderRegister;
	Constants.RegisterSpace = space;
	Constants.Num32BitValues = numConstants;
}

void DX12::RootParameter::AsCBV(D3D12_SHADER_VISIBILITY visibility, unsigned int shaderRegister, unsigned int space, D3D12_ROOT_DESCRIPTOR_FLAGS flags)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	ShaderVisibility = visibility;
	AsDescriptor(shaderRegister, space, flags);
}

void DX12::RootParameter::AsSRV(D3D12_SHADER_VISIBILITY visibility, unsigned int shaderRegister, unsigned int space, D3D12_ROOT_DESCRIPTOR_FLAGS flags)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	ShaderVisibility = visibility;
	AsDescriptor(shaderRegister, space, flags);
}

void DX12::RootParameter::AsUAV(D3D12_SHADER_VISIBILITY visibility, unsigned int shaderRegister, unsigned int space, D3D12_ROOT_DESCRIPTOR_FLAGS flags)
{
	ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	ShaderVisibility = visibility;
	AsDescriptor(shaderRegister, space, flags);
}

void DX12::RootParameter::AsDescriptorTable(D3D12_SHADER_VISIBILITY visibility, DX12::DescriptorRange* ranges, unsigned int count)
{
	CRITICAL_ASSERT(ranges, "Trying to create descriptor table without valid descriptor ranges.");

	ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ShaderVisibility = visibility;
	DescriptorTable.pDescriptorRanges = ranges;
	DescriptorTable.NumDescriptorRanges = count;
}

constexpr void DX12::RootParameter::AsDescriptor(unsigned int shaderRegister, unsigned int space, D3D12_ROOT_DESCRIPTOR_FLAGS flags)
{
	Descriptor.ShaderRegister = shaderRegister;
	Descriptor.RegisterSpace = space;
	Descriptor.Flags = flags;
}

ID3D12RootSignature* DX12::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC2& desc)
{
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
	versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_2;
	versionedDesc.Desc_1_2 = desc;

	Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

#ifdef _DEBUG
	HRESULT_CHECK_FALLBACK(
		D3D12SerializeVersionedRootSignature(&versionedDesc, &rootSignatureBlob, &errorBlob),
		const char* errorMsg = { errorBlob ? (const char*)errorBlob->GetBufferPointer() : "" },
		assert(false),
		"Root signature creation fail : %s",
		errorMsg);
#else
	HRESULT_CHECK_FALLBACK(
		D3D12SerializeVersionedRootSignature(&versionedDesc, &rootSignatureBlob, &errorBlob),,
		assert(false),
		"Root signature creation fail.");
#endif

	ID3D12RootSignature* signature = nullptr;
	HRESULT_CHECK_FALLBACK(
		DX12::GetRendererDevice()->CreateRootSignature(
			0,
			rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&signature)),,
		Utils::COM::SafeRelease(signature),
		"Could not create root signature.");

	return signature;
}

ID3D12PipelineState* DX12::CreatePipelineState(void* stream, unsigned long long int streamSize)
{
	CRITICAL_ASSERT(stream && streamSize > 0, "Cannot create pipeline state without valid stream.");

	D3D12_PIPELINE_STATE_STREAM_DESC desc{};
	desc.pPipelineStateSubobjectStream = stream;
	desc.SizeInBytes = streamSize;

	ID3D12PipelineState* state = nullptr;
	HRESULT_CHECK_FALLBACK(
		DX12::GetRendererDevice()->CreatePipelineState(&desc, IID_PPV_ARGS(&state)), ,
		assert(false),
		"Could not create pipeline state.");

	return state;
}

void DX12::RenderPassBase::BakeCommands(ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(commandList, "Cannot render the pass without a valid command list.");

	RenderPass(commandList);

	if (_next)
		_next->BakeCommands(commandList);
}
