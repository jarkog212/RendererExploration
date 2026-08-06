#include "DX12RenderTarget.h"
#include "DX12DescriptorHeap.h"

DX12::RenderTargetTexture::RenderTargetTexture(DX12::TextureSpecs& spec)
	: _texture(spec)
{
	_mipCount = _texture.GetResource()->GetDesc().MipLevels;
	
	if (_mipCount > DX12::Texture::_k_maxMips)
	{
		DISPLAY_ERROR(L"RTT resource has more MIPs than our declared maximum.");
		assert(false);
	}

	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = spec._resourceDesc->Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	ID3D12Device8* device = DX12::GetRendererDevice();
	DX12::DescriptorHeap* heap = DX12::GetRTVHeap();

	for (int i = 0; i < _mipCount; i++)
	{
		desc.Texture2D.MipSlice = i;
		_rtvHandles[i] = heap->Allocate();
		device->CreateRenderTargetView(_texture.GetResource(), &desc, _rtvHandles[i]._cpu);
	}
}

void DX12::RenderTargetTexture::Release()
{
	for (int i = 0; i < _mipCount; i++)
		DX12::GetRTVHeap()->Free(_rtvHandles[i]);
}

constexpr D3D12_CPU_DESCRIPTOR_HANDLE DX12::RenderTargetTexture::GetRTVCPUHandle(unsigned int mip)
{
	if (mip >= _mipCount)
	{
		DISPLAY_ERROR(L"Trying to get RTT's RTV handle with a out of bounds MIP index.")
		assert(false);
	}

	return _rtvHandles[mip]._cpu;
}

void DX12::RenderTargetTexture::MoveImpl(RenderTargetTexture&& other)
{
	assert(this != &other);

	_texture = std::move(other._texture);
	_mipCount = other._mipCount;
	for (int i = 0; i < _mipCount; i++)
		_rtvHandles[i] = std::move(other._rtvHandles[i]);

	other._mipCount = 0;
}
