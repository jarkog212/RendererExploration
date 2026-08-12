#include "DX12DepthBuffer.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

DX12::DepthBuffer DX12::DepthBuffer::Create(DX12::TextureSpecs& specs)
{
	CRITICAL_ASSERT(specs._resourceDesc, "Trying to make depth buffer with invalid resource descriptor.");
	CRITICAL_ASSERT(!specs._resource, "Trying to make depth buffer with non-empty DX12 resource.");

	DXGI_FORMAT stencilFormat = specs._resourceDesc->Format;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::k_depthBufferSRVTemplate;

	if (!specs._srvDesc)
	{
		if (specs._resourceDesc->Format == DXGI_FORMAT_D32_FLOAT)
		{
			specs._resourceDesc->Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else
		{
			srvDesc.Format = specs._resourceDesc->Format;
		}

		specs._srvDesc = &srvDesc;
	}
	return DepthBuffer(specs, stencilFormat);
}

DX12::DepthBuffer::DepthBuffer(DX12::TextureSpecs& specs, DXGI_FORMAT stencilFormat) :
	_texture(specs)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = DX12::CC::Desc::k_depthStencilViewTemplate;
	dsvDesc.Format = stencilFormat;

	_dsvHandle = DX12::GetDSVHeap()->Allocate();
	DX12::GetRendererDevice()->CreateDepthStencilView(_texture.GetResource(), &dsvDesc, _dsvHandle._cpu);
}

DX12::DepthBuffer::DepthBuffer(DepthBuffer&& other) noexcept :
	_texture(std::move(other._texture)),
	_dsvHandle(std::move(other._dsvHandle))
{
}

DX12::DepthBuffer& DX12::DepthBuffer::operator=(DepthBuffer&& other) noexcept
{
	assert(this != &other);

	_texture = std::move(other._texture);
	_dsvHandle = std::move(other._dsvHandle);

	return *this;
}

void DX12::DepthBuffer::Release()
{
	DX12::GetDSVHeap()->Free(_dsvHandle);
}
