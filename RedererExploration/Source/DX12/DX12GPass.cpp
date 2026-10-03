#include "DX12GPass.h"
#include "DX12Renderer.h"
#include "DX12Shaders.h"
#include "DX12Utils.h"

#include "DX12DescriptorHeap.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace 
{
	namespace RootSignature 
	{
		static Microsoft::WRL::ComPtr<ID3D12RootSignature> g_instance = nullptr;
		enum Params : unsigned int
		{
			Constants,
			SrvDescriptorTable,

			Count
		};
	}
	
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> g_GPassPipelineState = nullptr;

	static std::unique_ptr<DX12::DepthPass> g_DepthPass = nullptr;
	static std::unique_ptr<DX12::GPassRT> g_GPassRTPass = nullptr;

	constexpr D3D12_RESOURCE_STATES k_depthReadOnly =
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	constexpr D3D12_RESOURCE_STATES k_depthWriteOnly = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	static bool g_GPassLinked = false;

	void LinkGPassSubGraph()
	{
		DX12::GPassSpace::GetDepthPass()->SetNext(*DX12::GPassSpace::GetGPassRTPass());
		DX12::GPassSpace::GetGPassRTPass()->LinkDepthPass(DX12::GPassSpace::GetDepthPass());
		::g_GPassLinked = true;
	}
}

void DX12::GPassSpace::WarmUp()
{
	DX12::GPassSpace::GetPipelineState();
	DX12::GPassSpace::GetHeadNode();
}

void DX12::GPassSpace::Release()
{
	::RootSignature::g_instance.Reset();
	::g_GPassPipelineState.Reset();
	::g_DepthPass.reset();
	::g_GPassRTPass.reset();
	::g_GPassLinked = false;
}

ID3D12RootSignature* DX12::GPassSpace::GetRootSignature()
{
	if (::RootSignature::g_instance) [[likely]]
		return ::RootSignature::g_instance.Get();

	DX12::RootParameter params[::RootSignature::Params::Count]{};
	params[::RootSignature::Params::Constants].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);
	
	DX12::DescriptorRange srvDescriptors = DX12::GetSRVHeap()->AsDescriptorRange();
	srvDescriptors.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	srvDescriptors.BaseShaderRegister = 0;
	srvDescriptors.RegisterSpace = 0;
	params[::RootSignature::Params::SrvDescriptorTable].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &srvDescriptors, 1);

	D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	desc.NumParameters = _countof(params);
	desc.pParameters = params;

	::RootSignature::g_instance = DX12::Helpers::CreateRootSignature(desc);

	HRESULT_CHECK_CRITICAL(
		::RootSignature::g_instance->SetName(L"GPass Root Signature."),
		"Could not name GPass root signature.");

	return ::RootSignature::g_instance.Get();
}

ID3D12PipelineState* DX12::GPassSpace::GetPipelineState()
{
	if (::g_GPassPipelineState) [[likely]]
		return ::g_GPassPipelineState.Get();

	struct : public DX12::GPassSpace::PSOBase{
		PipelineStream::SubVertexShader _vs{ DX12::ShaderSpace::GetGenericShader(DX12::Shaders::GenericShader::Type::FullscreenTri_VS) };
		PipelineStream::SubPixelShader _ps{DX12::ShaderSpace::GetGenericShader(DX12::Shaders::GenericShader::Type::FillColor_PS) };
	}stream;

	::g_GPassPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		::g_GPassPipelineState->SetName(L"GPass PSO."),
		"Could not name GPass PSO.");

	return ::g_GPassPipelineState.Get();
}

DX12::DepthPass* DX12::GPassSpace::GetDepthPass(DirectX::XMUINT2 initSize)
{
	if (::g_DepthPass) [[likely]]
		return ::g_DepthPass.get();

	::g_DepthPass = std::make_unique<DX12::DepthPass>(DX12::DepthPass::Create(initSize));
	return ::g_DepthPass.get();
}

DX12::GPassRT* DX12::GPassSpace::GetGPassRTPass(DirectX::XMUINT2  initSize)
{
	if (::g_GPassRTPass) [[likely]]
		return ::g_GPassRTPass.get();

	::g_GPassRTPass = std::make_unique<DX12::GPassRT>(DX12::GPassRT::Create(initSize));
	return ::g_GPassRTPass.get();
}

DX12::RenderPassBase* DX12::GPassSpace::GetHeadNode()
{
	if(::g_GPassLinked) [[likely]]
		return DX12::GPassSpace::GetDepthPass();

	::LinkGPassSubGraph();

	return DX12::GPassSpace::GetDepthPass();
}

DX12::RenderPassBase* DX12::GPassSpace::GetTailNode()
{
	if (::g_GPassLinked) [[likely]]
		return DX12::GPassSpace::GetGPassRTPass();

	::LinkGPassSubGraph();

	return DX12::GPassSpace::GetGPassRTPass();
}

void DX12::GPassSpace::SetDimensions(const DirectX::XMUINT2& size)
{
	DX12::GPassSpace::GetDepthPass()->SetDimensions(size);
	DX12::GPassSpace::GetGPassRTPass()->SetDimensions(size);
}

DX12::DepthPass DX12::DepthPass::Create(const DirectX::XMUINT2& size)
{
	return DepthPass(DX12::DepthPass::CreateDepthBuffer(size));
}

DX12::DepthPass::DepthPass(DX12::DepthBufferTexture&& depthBuffer) : 
	_depthBuffer(std::move(depthBuffer))
{
	HRESULT_CHECK_CRITICAL(
		_depthBuffer.GetResource()->SetName(L"Depth Buffer"), "Could not name depthbuffer resource");
	
	_name = "GPass - Depth buffer";
}

DX12::DepthPass::DepthPass(DepthPass&& other) noexcept :
	RenderPassBase(std::move(other)),
	_depthBuffer(std::move(other._depthBuffer)),
	_dimensions(std::exchange(other._dimensions, { 0,0 }))
{
}

DX12::DepthPass& DX12::DepthPass::operator=(DepthPass&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving Depth Pass to itself.");

	RenderPassBase::operator=(std::move(other));

	std::swap(_depthBuffer, other._depthBuffer);
	std::swap(_dimensions, other._dimensions);

	return *this;
}

void DX12::DepthPass::PrePassResourceBarriers()
{
	DX12::PendingTransitionResourceBarrier(_depthBuffer.GetResource(), ::k_depthReadOnly, k_depthWriteOnly);
}

void DX12::DepthPass::AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList)
{
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = _depthBuffer.GetDSVCpuHandle();
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(0, nullptr, false, &dsv);

	commandList->SetGraphicsRootSignature(DX12::GPassSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::GPassSpace::GetPipelineState());

	// Further pass code here sending input to input assembler and then drawing the scene

}

void DX12::DepthPass::PostPassResourceBarriers()
{
	DX12::PendingTransitionResourceBarrier(_depthBuffer.GetResource(), k_depthWriteOnly, k_depthReadOnly);
}

void DX12::DepthPass::SetDimensions(const DirectX::XMUINT2& dimensions)
{
	DirectX::XMUINT2 newDimensions = dimensions;
	if (dimensions.x > _dimensions.x || dimensions.y > _dimensions.y)
	{
		newDimensions = { std::max(dimensions.x, _dimensions.x), std::max(dimensions.y, _dimensions.y) };
		_depthBuffer = DX12::DepthPass::CreateDepthBuffer(newDimensions);
	}

	_dimensions = newDimensions;
}

DX12::DepthBufferTexture DX12::DepthPass::CreateDepthBuffer(const DirectX::XMUINT2& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::GPassRes::k_depthTextureTemplate;
	desc.Width = size.x;
	desc.Height = size.y;

	DX12::Texture::Specs specs;
	specs._resourceDesc = &desc;
	specs._initialState =
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	specs._clearValue.Format = desc.Format;
	specs._clearValue.DepthStencil.Depth = 0.f;
	specs._clearValue.DepthStencil.Stencil = 0;

	return DX12::DepthBufferTexture::Create(specs);
}

DX12::GPassRT DX12::GPassRT::Create(const DirectX::XMUINT2& size)
{
	return DX12::GPassRT(DX12::GPassRT::CreateRenderTarget(size));
}

DX12::GPassRT::GPassRT(DX12::RenderTargetTexture&& mainBuffer) :
	_mainBuffer(std::move(mainBuffer))
{
	HRESULT_CHECK_CRITICAL(
		_mainBuffer.GetResource()->SetName(L"GPass Main Buffer"), "Could not name gpass main buffer resource");

	_name = "GPass - Render Target";
}

DX12::GPassRT::GPassRT(GPassRT&& other) noexcept :
	RenderPassBase(std::move(other)),
	_linkToDepthPass(std::exchange(other._linkToDepthPass, nullptr)),
	_mainBuffer(std::move(other._mainBuffer)),
	_dimensions(std::exchange(other._dimensions, { 0,0 }))
{
}

DX12::GPassRT& DX12::GPassRT::operator=(GPassRT&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving GPass RT to itself.");

	RenderPassBase::operator=(std::move(other));

	std::swap(_linkToDepthPass, other._linkToDepthPass);
	std::swap(_mainBuffer, other._mainBuffer);
	std::swap(_dimensions, other._dimensions);

	return *this;
}

void DX12::GPassRT::PrePassResourceBarriers()
{
	DX12::PendingTransitionResourceBarrier(
		_mainBuffer.GetResource(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DX12::GPassRT::AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(_linkToDepthPass, "Cannot render without a valid depth pass.")

	const D3D12_CPU_DESCRIPTOR_HANDLE rtv = _mainBuffer.GetRTVCPUHandle(0);
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = _linkToDepthPass->GetDepthBuffer()->GetDSVCpuHandle();

	commandList->ClearRenderTargetView(rtv, DX12::CC::k_clearValue, 0, nullptr);
	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
	
	static unsigned int call = 0;
	commandList->SetGraphicsRoot32BitConstant(::RootSignature::Params::Constants, ++call, 0);

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12::GPassRT::PostPassResourceBarriers()
{
	DX12::PendingTransitionResourceBarrier(
		_mainBuffer.GetResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DX12::GPassRT::SetDimensions(const DirectX::XMUINT2& dimensions)
{
	DirectX::XMUINT2 newDimensions = dimensions;
	if (dimensions.x > _dimensions.x || dimensions.y > _dimensions.y)
	{
		newDimensions = { std::max(dimensions.x, _dimensions.x), std::max(dimensions.y, _dimensions.y) };
		_mainBuffer = DX12::GPassRT::CreateRenderTarget(newDimensions);
	}

	_dimensions = newDimensions;
}

DX12::RenderTargetTexture DX12::GPassRT::CreateRenderTarget(const DirectX::XMUINT2& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::GPassRes::k_mainTextureTemplate;
	desc.Width = size.x;
	desc.Height = size.y;

	DX12::Texture::Specs specs;
	specs._resourceDesc = &desc;
	specs._initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	specs._clearValue.Format = desc.Format;
	memcpy(&specs._clearValue.Color, &DX12::CC::k_clearValue, sizeof(DX12::CC::k_clearValue));

	return DX12::RenderTargetTexture::Create(specs);

}


