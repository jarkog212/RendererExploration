#include "DX12GPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Renderer.h"
#include "DX12Shaders.h"
#include "DX12Utils.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace 
{
	static ID3D12RootSignature* g_GPassRootSignature = nullptr; // raw as it's cleanup is deferred
	static ID3D12PipelineState* g_GPassPipelineState = nullptr; // raw as it's cleanup is deferred

	static std::unique_ptr<DX12::DepthPass> g_DepthPass = nullptr;
	static std::unique_ptr<DX12::GPassRT> g_GPassRTPass = nullptr;

	constexpr D3D12_RESOURCE_STATES k_depthReadOnly =
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	constexpr D3D12_RESOURCE_STATES k_depthWriteOnly = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	static bool g_GPassLinked = false;

	void LinkGPassTree()
	{
		DX12::GPassSpace::GetDepthPass()->SetNext(*DX12::GPassSpace::GetGPassRTPass());
		DX12::GPassSpace::GetGPassRTPass()->LinkDepthPass(DX12::GPassSpace::GetDepthPass());
		::g_GPassLinked = true;
	}
}

void DX12::GPassSpace::WarmUp()
{
	DX12::GPassSpace::GetRootSignature();
	DX12::GPassSpace::GetPipelineState();
	DX12::GPassSpace::GetDepthPass(::k_defaultDimensions);
	DX12::GPassSpace::GetGPassRTPass(::k_defaultDimensions);
}

void DX12::GPassSpace::Release()
{
	Utils::COM::SafeDeferredRenderThreadRelease(::g_GPassRootSignature);
	Utils::COM::SafeDeferredRenderThreadRelease(::g_GPassPipelineState);
	::g_DepthPass.reset();
	::g_GPassRTPass.reset();
	::g_GPassLinked = false;
}

ID3D12RootSignature* DX12::GPassSpace::GetRootSignature()
{
	if (::g_GPassRootSignature) [[likely]]
		return ::g_GPassRootSignature;

	DX12::RootParameter params[1]{};
	params[0].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 0);
	
	D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	desc.NumParameters = _countof(params);
	desc.pParameters = params;

	::g_GPassRootSignature = DX12::Helpers::CreateRootSignature(desc);

	HRESULT_CHECK_CRITICAL(
		::g_GPassRootSignature->SetName(L"GPass Root Signature."),
		"Could not name GPass roost signature.");

	return ::g_GPassRootSignature;
}

ID3D12PipelineState* DX12::GPassSpace::GetPipelineState()
{
	if (::g_GPassPipelineState) [[likely]]
		return ::g_GPassPipelineState;

	using namespace DX12::PipelineStream;
	struct {	
		SubRootSignature rs{ DX12::GPassSpace::GetRootSignature() };
		SubVertexShader vs{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FullscreenTri_VS) };
		SubPixelShader ps{DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FillColor_PS) };
		SubPrimitiveTopology primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		SubRenderTargetFormats renderTargetFormats{};
		SubDepthStencilFormat depthStencilFormat{ DX12::CC::Desc::GPassRes::k_depthTextureTemplate.Format };
		SubRasterizer rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
		SubDepthStencil1 depth{ DX12::CC::Desc::DepthStencil::k_disabledBothTemplate };
	}stream;
	stream.renderTargetFormats._sub.NumRenderTargets = 1;
	stream.renderTargetFormats._sub.RTFormats[0] = DX12::CC::Desc::GPassRes::k_mainTextureTemplate.Format;

	::g_GPassPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		::g_GPassPipelineState->SetName(L"GPass PSO."),
		"Could not name GPass PSO.");

	return ::g_GPassPipelineState;
}

DX12::DepthPass* DX12::GPassSpace::GetDepthPass(DX12::Math::Vector2u32 size)
{
	if (::g_DepthPass) [[likely]]
		return ::g_DepthPass.get();

	::g_DepthPass = std::make_unique<DX12::DepthPass>(DX12::DepthPass::Create(size));
	return ::g_DepthPass.get();
}

DX12::GPassRT* DX12::GPassSpace::GetGPassRTPass(DX12::Math::Vector2u32 size)
{
	if (::g_GPassRTPass) [[likely]]
		return ::g_GPassRTPass.get();

	::g_GPassRTPass = std::make_unique<DX12::GPassRT>(DX12::GPassRT::Create(size));
	return ::g_GPassRTPass.get();
}

DX12::RenderPassBase* DX12::GPassSpace::GetHeadNode()
{
	if(::g_GPassLinked) [[likely]]
		return DX12::GPassSpace::GetDepthPass();

	::LinkGPassTree();

	return DX12::GPassSpace::GetDepthPass();
}

DX12::RenderPassBase* DX12::GPassSpace::GetTailNode()
{
	if (::g_GPassLinked) [[likely]]
		return DX12::GPassSpace::GetGPassRTPass();

	::LinkGPassTree();

	return DX12::GPassSpace::GetGPassRTPass();
}

void DX12::GPassSpace::SetDimensions(const DX12::Math::Vector2u32& size)
{
	DX12::GPassSpace::GetDepthPass()->SetDimensions(size);
	DX12::GPassSpace::GetGPassRTPass()->SetDimensions(size);
}

DX12::DepthPass DX12::DepthPass::Create(const DX12::Math::Vector2u32& size)
{
	return DepthPass(DX12::DepthPass::CreateDepthBuffer(size));
}

DX12::DepthPass::DepthPass(DX12::DepthBufferTexture&& depthBuffer) : 
	_depthBuffer(std::move(depthBuffer))
{
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

	// Further pass code here

}

void DX12::DepthPass::PostPassResourceBarriers()
{
	DX12::PendingTransitionResourceBarrier(_depthBuffer.GetResource(), k_depthWriteOnly, k_depthReadOnly);
}

void DX12::DepthPass::SetDimensions(const DX12::Math::Vector2u32& dimensions)
{
	DX12::Math::Vector2u32 newDimensions = dimensions;
	if (dimensions.X() > _dimensions.X() || dimensions.X() > _dimensions.Y())
	{
		newDimensions = { std::max(dimensions.X(), _dimensions.X()), std::max(dimensions.Y(), _dimensions.Y()) };
		_depthBuffer = DX12::DepthPass::CreateDepthBuffer(newDimensions);
	}

	_dimensions = newDimensions;
}

DX12::DepthBufferTexture DX12::DepthPass::CreateDepthBuffer(const DX12::Math::Vector2u32& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::GPassRes::k_depthTextureTemplate;
	desc.Width = size.X();
	desc.Height = size.Y();

	DX12::TextureSpecs specs;
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

DX12::GPassRT DX12::GPassRT::Create(const DX12::Math::Vector2u32& size)
{
	return DX12::GPassRT(DX12::GPassRT::CreateRenderTarget(size));
}

DX12::GPassRT::GPassRT(DX12::RenderTargetTexture&& mainBuffer) :
	_mainBuffer(std::move(mainBuffer))
{
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

	commandList->SetGraphicsRootSignature(DX12::GPassSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::GPassSpace::GetPipelineState());
	
	static unsigned int call = 0;
	commandList->SetGraphicsRoot32BitConstant(0, ++call, 0);

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

void DX12::GPassRT::SetDimensions(const DX12::Math::Vector2u32& dimensions)
{
	DX12::Math::Vector2u32 newDimensions = dimensions;
	if (dimensions.X() > _dimensions.X() || dimensions.Y() > _dimensions.Y())
	{
		newDimensions = { std::max(dimensions.X(), _dimensions.X()), std::max(dimensions.Y(), _dimensions.Y()) };
		_mainBuffer = DX12::GPassRT::CreateRenderTarget(newDimensions);
	}

	_dimensions = newDimensions;
}

DX12::RenderTargetTexture DX12::GPassRT::CreateRenderTarget(const DX12::Math::Vector2u32& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::GPassRes::k_mainTextureTemplate;
	desc.Width = size.X();
	desc.Height = size.Y();

	DX12::TextureSpecs specs;
	specs._resourceDesc = &desc;
	specs._initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	specs._clearValue.Format = desc.Format;
	memcpy(&specs._clearValue.Color, &DX12::CC::k_clearValue, sizeof(DX12::CC::k_clearValue));

	return DX12::RenderTargetTexture::Create(specs);

}


