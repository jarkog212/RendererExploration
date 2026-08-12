#include "DX12RenderTarget.h"
#include "DX12DescriptorHeap.h"
#include "DX12CommonDescriptors.h"

DX12::RenderTargetTexture DX12::RenderTargetTexture::Create(DX12::TextureSpecs& specs)
{
	CRITICAL_ASSERT(specs._resourceDesc, "Trying to make render target with invalid resource descriptor.");
	CRITICAL_ASSERT(!specs._resource, "Trying to make render target with non-empty DX12 resource.");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::k_renderTargetSRVTemplate;
	if (!specs._srvDesc)
		specs._srvDesc = &srvDesc;

	return RenderTargetTexture(specs);
}

DX12::RenderTargetTexture::RenderTargetTexture(DX12::TextureSpecs& specs) :
	_texture(specs)
{
	_mipCount = _texture.GetResource()->GetDesc().MipLevels;
	
	CRITICAL_ASSERT(
		_mipCount <= DX12::Texture::_k_maxMips,
		"RTT resource has more MIPs than our declared maximum.");

	D3D12_RENDER_TARGET_VIEW_DESC desc = DX12::CC::Desc::k_renderTargetViewTemplate;
	desc.Format = specs._resourceDesc->Format;

	DX12::DescriptorHeap* rtvHeap = DX12::GetRTVHeap();
	for (int i = 0; i < _mipCount; i++)
	{
		desc.Texture2D.MipSlice = i;
		_rtvHandles[i] = rtvHeap->Allocate();
		DX12::GetRendererDevice()->CreateRenderTargetView(_texture.GetResource(), &desc, _rtvHandles[i]._cpu);
	}
}

DX12::RenderTargetTexture::RenderTargetTexture(RenderTargetTexture&& other) noexcept :
	_texture(std::move(other._texture)),
	_mipCount(std::exchange(other._mipCount, 0))
{
	for (int i = 0; i < _mipCount; i++)
		_rtvHandles[i] = std::move(other._rtvHandles[i]);
}

DX12::RenderTargetTexture& DX12::RenderTargetTexture::operator=(RenderTargetTexture&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving RenderTargetTexture to itself.");

	std::swap(_texture, other._texture);
	std::swap(_mipCount, other._mipCount);
	for (int i = 0; i < _mipCount; i++)
		std::swap(_rtvHandles[i], other._rtvHandles[i]);

	return *this;
}

void DX12::RenderTargetTexture::Release()
{
	DX12::DescriptorHeap* rtvHeap = DX12::GetRTVHeap();
	for (int i = 0; i < _mipCount; i++)
		rtvHeap->Free(_rtvHandles[i]);
}

constexpr D3D12_CPU_DESCRIPTOR_HANDLE DX12::RenderTargetTexture::GetRTVCPUHandle(unsigned int mip) const
{
	CRITICAL_ASSERT(
		mip < _mipCount,
		"Trying to get RTT's RTV handle with a out of bounds MIP index.");

	return _rtvHandles[mip]._cpu;
}
