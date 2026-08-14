#include "DX12PostProcessPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Utils.h"
#include "Utils.h"


namespace 
{
	static ID3D12RootSignature* g_PPRootSignature = nullptr; // raw as it's cleanup is deferred
	static ID3D12PipelineState* g_PPPipelineState = nullptr; // raw as it's cleanup is deferred
}

void DX12::PostProcessSpace::WarmUp()
{
	DX12::PostProcessSpace::GetRootSignature();
	DX12::PostProcessSpace::GetPipelineState();
}

void DX12::PostProcessSpace::Release()
{
	Utils::COM::SafeDeferredRenderThreadRelease(::g_PPRootSignature);
	Utils::COM::SafeDeferredRenderThreadRelease(::g_PPRootSignature);
}

ID3D12RootSignature* DX12::PostProcessSpace::GetRootSignature()
{
	if (::g_PPRootSignature) [[likely]]
		return ::g_PPRootSignature;

		//D3D12_ROOT_SIGNATURE_DESC2 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
		//desc.NumParameters = _countof(params);
		//desc.pParameters = params;

		//::g_PPRootSignature = DX12::Helpers::CreateRootSignature(desc);

		HRESULT_CHECK_CRITICAL(
			::g_PPRootSignature->SetName(L"Post Process Root Signature."),
			"Could not name Post Process roost signature.");

		return ::g_PPRootSignature;
}

ID3D12PipelineState* DX12::PostProcessSpace::GetPipelineState()
{
	if (::g_PPPipelineState) [[likely]]
		return ::g_PPPipelineState;

		//D3D12_RT_FORMAT_ARRAY renderTargetFormats;
		//renderTargetFormats.NumRenderTargets = 1;
		//renderTargetFormats.RTFormats[0] = DX12::CC::Desc::GPassRes::k_mainTextureTemplate.Format;

		using namespace DX12::PipelineStream;
		struct {
			SubRootSignature rs{ DX12::PostProcessSpace::GetRootSignature() };
			//SubVertexShader vs{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FullscreenTri_VS) };
			//SubPixelShader ps{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FillColor_PS) };
			//SubPrimitiveTopology primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
			//SubRenderTargetFormats renderTargetFormats{ renderTargetFormats };
			//SubDepthStencilFormat depthStencilFormat{ DX12::CC::Desc::GPassRes::k_depthTextureTemplate.Format };
			//SubRasterizer rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
			//SubDepthStencil1 depth{ DX12::CC::Desc::DepthStencil::k_disabledBothTemplate };
		}stream;

		::g_PPPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

		HRESULT_CHECK_CRITICAL(
			::g_PPPipelineState->SetName(L"Post process PSO."),
			"Could not name Post process PSO.");

		return ::g_PPPipelineState;
}

void DX12::PostProcessPass::PrePassResourceBarriers()
{
}

void DX12::PostProcessPass::AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList)
{
}

void DX12::PostProcessPass::PostPassResourceBarriers()
{
}
