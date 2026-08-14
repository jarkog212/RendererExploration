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

void DX12::RenderPassBase::AccummulateCommands(ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(commandList, "Cannot render the pass without a valid command list.");

	AccummulatePassCommands(commandList);

	if (_next)
		_next->AccummulateCommands(commandList);
}
