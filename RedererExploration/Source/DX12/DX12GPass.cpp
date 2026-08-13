#include "DX12GPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Renderer.h"
#include "Dx12Shaders.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace 
{
#ifdef _DEBUG
	constexpr float k_clearValue[4]{ 0.5f, 0.5f, 0.5f, 1.f };
#else
	constexpr float k_clearValue[4]{};
#endif

	static ID3D12RootSignature* GPassRootSignature = nullptr; // raw as it's cleanup is deferred
	static ID3D12PipelineState* GPassPipelineState = nullptr; // raw as it's cleanup is deferred
}

void DX12::GPassSpace::WarmUp()
{
	DX12::GPassSpace::GetRootSignature();
	DX12::GPassSpace::GetPipelineState();
}

void DX12::GPassSpace::Release()
{
	Utils::COM::SafeDeferredRenderThreadRelease(::GPassRootSignature);
	Utils::COM::SafeDeferredRenderThreadRelease(::GPassPipelineState);
}

ID3D12RootSignature* DX12::GPassSpace::GetRootSignature()
{
	if (::GPassRootSignature) [[likely]]
		return ::GPassRootSignature;

	DX12::RootParameter params[1]{};
	params[0].AsConstants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
	
	D3D12_ROOT_SIGNATURE_DESC2 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
	desc.NumParameters = _countof(params);
	desc.pParameters = params;

	::GPassRootSignature = DX12::CreateRootSignature(desc);

	HRESULT_CHECK_CRITICAL(
		::GPassRootSignature->SetName(L"GPass Root Signature."),
		"Could not name GPass roost signature.");

	return ::GPassRootSignature;
}

ID3D12PipelineState* DX12::GPassSpace::GetPipelineState()
{
	if (::GPassPipelineState) [[likely]]
		return ::GPassPipelineState;

	D3D12_RT_FORMAT_ARRAY renderTargetFormats;
	renderTargetFormats.NumRenderTargets = 1;
	renderTargetFormats.RTFormats[0] = DX12::CC::Desc::GPassRes::k_mainTextureTemplate.Format;

	using namespace DX12::PipelineStream;
	struct {	
		SubRootSignature rs{ DX12::GPassSpace::GetRootSignature() };
		SubVertexShader vs{ DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FullscreenTri_VS) };
		SubPixelShader ps{DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::FillColor_PS) };
		SubPrimitiveTopology primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		SubRenderTargetFormats renderTargetFormats{ renderTargetFormats };
		SubDepthStencilFormat depthStencilFormat{ DX12::CC::Desc::GPassRes::k_depthTextureTemplate.Format };
		SubRasterizer rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
		SubDepthStencil1 depth{ DX12::CC::Desc::DepthStencil::k_disabledBothTemplate };
	}stream;

	::GPassPipelineState = DX12::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		::GPassPipelineState->SetName(L"GPass PSO."),
		"Could not name GPass PSO.");

	return ::GPassPipelineState;
}

DX12::GPass DX12::GPass::Create(DX12::Math::Vector2u32& size)
{
	return GPass(
		DX12::GPass::CreateMainBufferTemplate(size),
		DX12::GPass::CreateDepthBufferTemplate(size));
}

DX12::GPass::GPass(DX12::RenderTargetTexture&& mainBuffer, DX12::DepthBufferTexture&& depthBuffer) :
	_mainBuffer(std::move(mainBuffer)),
	_depthBuffer(std::move(depthBuffer))
{
	_name = "GPass";
}

DX12::GPass::GPass(GPass&& other) noexcept :
	RenderPassBase(std::move(other)),
	_mainBuffer(std::move(other._mainBuffer)),
	_depthBuffer(std::move(other._depthBuffer)),
	_dimensions(std::exchange(other._dimensions, { 0,0 }))
{
}

DX12::GPass& DX12::GPass::operator=(GPass&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving GPass to itself.");

	RenderPassBase::operator=(std::move(other));

	std::swap(_mainBuffer,other._mainBuffer);
	std::swap(_depthBuffer, other._depthBuffer);
	std::swap(_dimensions, other._dimensions);

	return *this;
}

void DX12::GPass::RenderPass(ID3D12GraphicsCommandList6* commandList)
{
	commandList->SetGraphicsRootSignature(DX12::GPassSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::GPassSpace::GetPipelineState());
	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	static unsigned int call = 0;
	commandList->SetGraphicsRoot32BitConstant(0, ++call, 0);

	DepthPass(commandList);
}

void DX12::GPass::SetDimensions(DX12::Math::Vector2u32 dimensions)
{
	DX12::Math::Vector2u32 newDimensions = dimensions;
	if (dimensions.X() > _dimensions.X() || dimensions.X() > _dimensions.Y())
	{
		newDimensions = { std::max(dimensions.X(), _dimensions.X()), std::max(dimensions.Y(), _dimensions.Y()) };
		_mainBuffer = DX12::GPass::CreateMainBufferTemplate(newDimensions);
		_depthBuffer = DX12::GPass::CreateDepthBufferTemplate(newDimensions);
	}

	_dimensions = newDimensions;
}

DX12::RenderTargetTexture DX12::GPass::CreateMainBufferTemplate(DX12::Math::Vector2u32& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::GPassRes::k_mainTextureTemplate;
	desc.Width = size.X();
	desc.Height = size.Y();

	DX12::TextureSpecs specs;
	specs._resourceDesc = &desc;
	specs._initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	specs._clearValue.Format = desc.Format;
	memcpy(&specs._clearValue.Color, &::k_clearValue, sizeof(::k_clearValue));

	return DX12::RenderTargetTexture::Create(specs);
}

DX12::DepthBufferTexture DX12::GPass::CreateDepthBufferTemplate(DX12::Math::Vector2u32& size)
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

void DX12::GPass::DepthPass(ID3D12GraphicsCommandList6* commandList)
{
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = _depthBuffer.GetDSVCpuHandle();
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(0, nullptr, false, &dsv);

	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12::GPass::FullPass(ID3D12GraphicsCommandList6* commandList)
{
	const D3D12_CPU_DESCRIPTOR_HANDLE dsv = _depthBuffer.GetDSVCpuHandle();
	const D3D12_CPU_DESCRIPTOR_HANDLE rtv = _mainBuffer.GetRTVCPUHandle(0);

	commandList->ClearRenderTargetView(rtv, k_clearValue, 0, nullptr);
	commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
}

