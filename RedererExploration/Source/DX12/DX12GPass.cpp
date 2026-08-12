#include "DX12GPass.h"
#include "DX12CommonDescriptors.h"

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
	D3D12_RESOURCE_DESC mainDesc = DX12::CC::Desc::k_GPassMainTextureTemplate;
	mainDesc.Width = size.X();
	mainDesc.Height = size.Y();

	DX12::TextureSpecs mainSpecs;
	mainSpecs._resourceDesc = &mainDesc;
	mainSpecs._initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	mainSpecs._clearValue.Format = mainDesc.Format;
	memcpy(&mainSpecs._clearValue.Color, &::k_clearValue, sizeof(::k_clearValue));

	D3D12_RESOURCE_DESC depthDesc = DX12::CC::Desc::k_GPassDepthTextureTemplate;
	depthDesc.Width = size.X();
	depthDesc.Height = size.Y();

	DX12::TextureSpecs depthSpecs;
	depthSpecs._resourceDesc = &depthDesc;
	depthSpecs._initialState =
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	depthSpecs._clearValue.Format = depthDesc.Format;
	depthSpecs._clearValue.DepthStencil.Depth = 0.f;
	depthSpecs._clearValue.DepthStencil.Stencil = 0;

	return GPass(mainSpecs, depthSpecs);
}

DX12::GPass::GPass(DX12::TextureSpecs& mainBufferSpecs, DX12::TextureSpecs& depthBufferSpecs) :
	_mainBuffer(DX12::RenderTargetTexture::Create(mainBufferSpecs)),
	_depthBuffer(DX12::DepthBuffer::Create(depthBufferSpecs))
{
}

DX12::GPass::GPass(GPass&& other) noexcept :
	_mainBuffer(std::move(other._mainBuffer)),
	_depthBuffer(std::move(other._depthBuffer)),
	_dimensions(std::exchange(other._dimensions, { 0,0 }))
{
}

DX12::GPass& DX12::GPass::operator=(GPass&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving GPass to itself.");

	std::swap(_mainBuffer,other._mainBuffer);
	std::swap(_depthBuffer, other._depthBuffer);
	std::swap(_dimensions, other._dimensions);

	return *this;
}
