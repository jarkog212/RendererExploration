#include "DX12DepthBuffer.h"
#include "DX12DescriptorHeap.h"

DX12::DepthBuffer::DepthBuffer(DX12::TextureSpecs& specs)
{
	CRITICAL_ASSERT(specs._resourceDesc, "Trying to make depth buffer with invalid resource descriptor.");
	CRITICAL_ASSERT(!specs._srvDesc, "Trying to make depth buffer with non-empty SRV descriptor.");
	CRITICAL_ASSERT(!specs._resource, "Trying to make depth buffer with non-empty DX12 resource.");

	DXGI_FORMAT stencilFormat = specs._resourceDesc->Format;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	
	if (specs._resourceDesc->Format == DXGI_FORMAT_D32_FLOAT)
	{
		specs._resourceDesc->Format = DXGI_FORMAT_R32_FLOAT;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

	specs._srvDesc = &srvDesc;
	_texture = DX12::Texture(specs);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = stencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;

	_dsvHandle = DX12::GetDSVHeap()->Allocate();
	DX12::GetRendererDevice()->CreateDepthStencilView(_texture.GetResource(), &dsvDesc, _dsvHandle._cpu);

}

void DX12::DepthBuffer::Release()
{
	DX12::GetDSVHeap()->Free(_dsvHandle);
}

void DX12::DepthBuffer::MoveImpl(DepthBuffer&& other)
{
	assert(this != &other);

	_texture = std::move(other._texture);
	_dsvHandle = std::move(other._dsvHandle);
}
