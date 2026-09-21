#include "DX12DepthBufferTexture.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"
#include "DX12Utils.h"

DX12::DepthBufferTexture DX12::DepthBufferTexture::Create(DX12::TextureSpecs& specs)
{
	CRITICAL_ASSERT(specs._resourceDesc, "Trying to make depth buffer with invalid resource descriptor.");
	CRITICAL_ASSERT(!specs._resource, "Trying to make depth buffer with non-empty DX12 resource.");

	DXGI_FORMAT depthStencilFormat = specs._resourceDesc->Format;
	
	if (!specs._srvDesc)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::DepthStencil::k_SRVTemplate;
		srvDesc.Format = DX12::Helpers::ConvertDepthDSVtoSRVFormat(specs._resourceDesc->Format);
		specs._srvDesc = &srvDesc;
	}

	// depth/stencil will always be 32bit per pixel, layout may be custom but the resource does not care beyond max width. 
	specs._resourceDesc->Format = DXGI_FORMAT_R32_TYPELESS;

	return DepthBufferTexture(specs, depthStencilFormat);
}

DX12::DepthBufferTexture::DepthBufferTexture(DX12::TextureSpecs& specs, DXGI_FORMAT depthStencilFormat) :
	_texture(specs)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = DX12::CC::Desc::DepthStencil::k_viewTemplate;
	dsvDesc.Format = depthStencilFormat;

	_dsvHandle = DX12::GetDSVHeap()->Allocate();
	DX12::GetRendererDevice()->CreateDepthStencilView(_texture.GetResource(), &dsvDesc, _dsvHandle._cpu);
}

DX12::DepthBufferTexture::DepthBufferTexture(DepthBufferTexture&& other) noexcept :
	_texture(std::move(other._texture)),
	_dsvHandle(std::move(other._dsvHandle))
{
}

DX12::DepthBufferTexture& DX12::DepthBufferTexture::operator=(DepthBufferTexture&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving Depth buffer to itself.");

	std::swap(_texture, other._texture);
	std::swap(_dsvHandle, other._dsvHandle);

	return *this;
}
