#include "DX12GPass.h"
#include "DX12CommonDescriptors.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace 
{
	constexpr DXGI_FORMAT k_defaultDepthFormat = DXGI_FORMAT_D32_FLOAT;

#ifdef _DEBUG
	constexpr float k_clearValue[4]{ 0.5f, 0.5f, 0.5f, 1.f };
#else
	constexpr float k_clearValue[4]{};
#endif
}

DX12::GPass DX12::GPass::Create(DX12::Math::Vector2u32& size)
{
	return GPass(
		DX12::GPass::CreateMainBufferTemplate(size),
		DX12::GPass::CreateDepthBufferTemplate(size));
}

DX12::GPass::GPass(DX12::RenderTargetTexture&& mainBuffer, DX12::DepthBuffer&& depthBuffer) :
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

void DX12::GPass::RenderPass(DX12::GfxCommandExecutor& commandExecutor)
{
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
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::k_GPassMainTextureTemplate;
	desc.Width = size.X();
	desc.Height = size.Y();

	DX12::TextureSpecs specs;
	specs._resourceDesc = &desc;
	specs._initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	specs._clearValue.Format = desc.Format;
	memcpy(&specs._clearValue.Color, &::k_clearValue, sizeof(::k_clearValue));

	return DX12::RenderTargetTexture::Create(specs);
}

DX12::DepthBuffer DX12::GPass::CreateDepthBufferTemplate(DX12::Math::Vector2u32& size)
{
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::k_GPassDepthTextureTemplate;
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

	return DX12::DepthBuffer::Create(specs);
}

