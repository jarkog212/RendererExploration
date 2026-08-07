#include "DX12RenderTarget.h"
#include "DX12DescriptorHeap.h"

DX12::RenderTargetTexture::RenderTargetTexture(DX12::TextureSpecs& spec)
	: _texture(spec)
{
	_mipCount = _texture.GetResource()->GetDesc().MipLevels;
	
	CRITICAL_ASSERT(
		_mipCount <= DX12::Texture::_k_maxMips,
		"RTT resource has more MIPs than our declared maximum.");

	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = spec._resourceDesc->Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	DX12::DescriptorHeap* rtvHeap = DX12::GetRTVHeap();
	for (int i = 0; i < _mipCount; i++)
	{
		desc.Texture2D.MipSlice = i;
		_rtvHandles[i] = rtvHeap->Allocate();
		DX12::GetRendererDevice()->CreateRenderTargetView(_texture.GetResource(), &desc, _rtvHandles[i]._cpu);
	}
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

void DX12::RenderTargetTexture::MoveImpl(RenderTargetTexture&& other)
{
	CRITICAL_ASSERT(this != &other, "Moving RenderTargetTexture to itself.");

	_texture = std::move(other._texture);
	_mipCount = other._mipCount;
	for (int i = 0; i < _mipCount; i++)
		_rtvHandles[i] = std::move(other._rtvHandles[i]);

	other._mipCount = 0;
}
