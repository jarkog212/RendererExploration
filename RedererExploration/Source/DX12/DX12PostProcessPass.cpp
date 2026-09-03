#include "DX12PostProcessPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Utils.h"
#include "DX12Shaders.h"
#include "DX12DescriptorHeap.h"
#include "DX12RenderSurface.h"
#include "DX12GPass.h"
#include "Utils.h"

namespace 
{
	namespace RootSignature
	{
		static Microsoft::WRL::ComPtr<ID3D12RootSignature> g_instance = nullptr;
		enum Params : unsigned int
		{
			Constants,
			DescriptorTables,

			Count
		};
	}
	
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> g_PPPipelineState = nullptr;

	static std::unique_ptr<DX12::PostProcessPass> g_postProcessPass = nullptr;
}

void DX12::PostProcessSpace::WarmUp()
{
	DX12::PostProcessSpace::GetPipelineState();
	DX12::PostProcessSpace::GetPostProcessPass();
}

void DX12::PostProcessSpace::Release()
{
	::RootSignature::g_instance.Reset();
	::g_PPPipelineState.Reset();
	::g_postProcessPass.reset();
}

ID3D12RootSignature* DX12::PostProcessSpace::GetRootSignature()
{
	if (::RootSignature::g_instance) [[likely]]
		return ::RootSignature::g_instance.Get();

	DX12::RootParameter params[::RootSignature::Params::Count]{};
	params[::RootSignature::Params::Constants].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);

	DX12::DescriptorRange srvDescriptors = DX12::GetSRVHeap()->AsDescriptorRange();
	srvDescriptors.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvDescriptors.BaseShaderRegister = 0;
	srvDescriptors.RegisterSpace = 0;
	params[::RootSignature::Params::DescriptorTables].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &srvDescriptors, 1);

	D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
	desc.NumParameters = _countof(params);
	desc.pParameters = params;

	::RootSignature::g_instance = DX12::Helpers::CreateRootSignature(desc);

	HRESULT_CHECK_CRITICAL(
		::RootSignature::g_instance->SetName(L"Post Process Root Signature."),
		"Could not name Post Process root signature.");

	return ::RootSignature::g_instance.Get();
}

ID3D12PipelineState* DX12::PostProcessSpace::GetPipelineState()
{
	if (::g_PPPipelineState) [[likely]]
		return ::g_PPPipelineState.Get();

	using namespace DX12::PipelineStream;
	struct {
		SubRootSignature _rs{ DX12::PostProcessSpace::GetRootSignature() };
		SubVertexShader _vs{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::Type::FullscreenTri_VS) };
		SubPixelShader _ps{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::Type::PostProcess_PS) };
		SubPrimitiveTopology _primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		SubRenderTargetFormats _renderTargetFormats{};
		SubRasterizer _rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
	}stream;
	stream._renderTargetFormats._sub.NumRenderTargets = 1;
	stream._renderTargetFormats._sub.RTFormats[0] = DX12::CC::k_defaultRenderTargetFormat;

	::g_PPPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		::g_PPPipelineState->SetName(L"Post process PSO."),
		"Could not name Post process PSO.");

	return ::g_PPPipelineState.Get();
}

DX12::PostProcessPass* DX12::PostProcessSpace::GetPostProcessPass()
{
	if (::g_postProcessPass) [[likely]]
		return ::g_postProcessPass.get();
		
	::g_postProcessPass = std::make_unique<DX12::PostProcessPass>();
	return  ::g_postProcessPass.get();
}

DX12::PostProcessPass::PostProcessPass(PostProcessPass&& other) noexcept :
	_renderSurface(std::exchange(other._renderSurface, nullptr)),
	_inputGPass(std::exchange(other._inputGPass, nullptr))
{
}

DX12::PostProcessPass& DX12::PostProcessPass::operator=(PostProcessPass&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Trying to move Post process pass to itself.");

	std::swap(_renderSurface, other._renderSurface);
	std::swap(_inputGPass, other._inputGPass);

	return *this;
}

void DX12::PostProcessPass::PrePassResourceBarriers()
{
	CRITICAL_ASSERT(_renderSurface, "Cannot declare post process pass's barriers without valid output surface.");

	DX12::PendingTransitionResourceBarrier(
		_renderSurface->GetBackbuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
}

void DX12::PostProcessPass::AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(_renderSurface, "Cannot do post process pass without valid output surface.");
	CRITICAL_ASSERT(_inputGPass, "Cannot do post process pass without valid GPass");

	commandList->SetGraphicsRootSignature(DX12::PostProcessSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::PostProcessSpace::GetPipelineState());

	DX12::DescriptorHeap* srvHeap = DX12::GetSRVHeap();
	ID3D12DescriptorHeap* heaps[]{ srvHeap->GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetGraphicsRoot32BitConstant(::RootSignature::Params::Constants, _inputGPass->GetMainBufferHandleView()._index, 0);
	commandList->SetGraphicsRootDescriptorTable(::RootSignature::Params::DescriptorTables, srvHeap->GetGpuPtr());

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const DX12::DescriptorHandleView surfaceRTHandle = _renderSurface->GetBackbufferHandle();
	commandList->OMSetRenderTargets(1, &surfaceRTHandle._cpu, true, nullptr);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12::PostProcessPass::PostPassResourceBarriers()
{
}
