#include "DX12PostProcessPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Utils.h"
#include "DX12Shaders.h"
#include "DX12DescriptorHeap.h"
#include "Utils.h"

namespace 
{
	namespace RootSignature
	{
		static ID3D12RootSignature* g_instance = nullptr; // raw as it's cleanup is deferred
		enum Params : unsigned int
		{
			Constants,
			DescriptorTables,

			Count
		};
	}
	
	static ID3D12PipelineState* g_PPPipelineState = nullptr; // raw as it's cleanup is deferred

	static std::unique_ptr<DX12::PostProcessPass> g_postProcessPass = nullptr;
}

void DX12::PostProcessSpace::WarmUp()
{
	DX12::PostProcessSpace::GetRootSignature();
	DX12::PostProcessSpace::GetPipelineState();
	DX12::PostProcessSpace::GetPostProcessPass();
}

void DX12::PostProcessSpace::Release()
{
	Utils::COM::SafeDeferredRenderThreadRelease(::RootSignature::g_instance);
	Utils::COM::SafeDeferredRenderThreadRelease(::g_PPPipelineState);
	g_postProcessPass.reset();
}

ID3D12RootSignature* DX12::PostProcessSpace::GetRootSignature()
{
	if (::RootSignature::g_instance) [[likely]]
		return ::RootSignature::g_instance;

		DX12::RootParameter params[::RootSignature::Params::Count]{};
		params[::RootSignature::Params::Constants].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);

		DX12::DescriptorRange srvDescriptors = DX12::GetSRVHeap()->AsDescriptorRange();
		srvDescriptors.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		params[::RootSignature::Params::DescriptorTables].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &srvDescriptors, 1);

		D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
		desc.NumParameters = _countof(params);
		desc.pParameters = params;

		::RootSignature::g_instance = DX12::Helpers::CreateRootSignature(desc);

		HRESULT_CHECK_CRITICAL(
			::RootSignature::g_instance->SetName(L"Post Process Root Signature."),
			"Could not name Post Process root signature.");

		return ::RootSignature::g_instance;
}

ID3D12PipelineState* DX12::PostProcessSpace::GetPipelineState()
{
	if (::g_PPPipelineState) [[likely]]
		return ::g_PPPipelineState;

		using namespace DX12::PipelineStream;
		struct {
			SubRootSignature rs{ DX12::PostProcessSpace::GetRootSignature() };
			SubVertexShader vs{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FullscreenTri_VS) };
			SubPixelShader ps{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::PostProcess_PS) };
			SubPrimitiveTopology primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
			SubRenderTargetFormats renderTargetFormats{};
			SubRasterizer rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
		}stream;
		stream.renderTargetFormats._sub.NumRenderTargets = 1;
		stream.renderTargetFormats._sub.RTFormats[0] = DX12::CC::k_defaultRenderTargetFormat;

		::g_PPPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

		HRESULT_CHECK_CRITICAL(
			::g_PPPipelineState->SetName(L"Post process PSO."),
			"Could not name Post process PSO.");

		return ::g_PPPipelineState;
}

DX12::PostProcessPass* DX12::PostProcessSpace::GetPostProcessPass()
{
	if (g_postProcessPass) [[likely]]
		return g_postProcessPass.get();
		
	g_postProcessPass = std::make_unique<DX12::PostProcessPass>();
	return  g_postProcessPass.get();
}

void DX12::PostProcessPass::PrePassResourceBarriers()
{
}

void DX12::PostProcessPass::AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(_renderTagetRTV.IsValid(), "Cannot do post process pass without valid output RTV");
	CRITICAL_ASSERT(_inputSRV.IsValid(), "Cannot do post process pass without valid input SRV");

	commandList->SetGraphicsRootSignature(DX12::PostProcessSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::PostProcessSpace::GetPipelineState());

	DX12::DescriptorHeap* srvHeap = DX12::GetSRVHeap();
	ID3D12DescriptorHeap* heaps[]{ srvHeap->GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetGraphicsRoot32BitConstant(::RootSignature::Params::Constants, _inputSRV._index, 0);
	commandList->SetGraphicsRootDescriptorTable(::RootSignature::Params::DescriptorTables, srvHeap->GetGpuPtr());

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->OMSetRenderTargets(1, &_renderTagetRTV._cpu, true, nullptr);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12::PostProcessPass::PostPassResourceBarriers()
{
}
