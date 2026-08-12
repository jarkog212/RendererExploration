#pragma once

#include "DX12Core.h"

namespace DX12::CC::Desc
{
	constexpr D3D12_HEAP_PROPERTIES k_heapPropertiesTemplate{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,	// CreationNodeMask
		0	// VisibleNodeMask
	};

	constexpr D3D12_SHADER_RESOURCE_VIEW_DESC k_renderTargetSRVTemplate{
		DXGI_FORMAT_UNKNOWN,
		D3D12_SRV_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		{.Texture2D = D3D12_TEX2D_SRV{
			0,		// MostDetailedMip
			0,		// MipLevels;
			0,		// PlaneSlice;
			0.f		// ResourceMinLODClamp;
		}}
	};

	constexpr D3D12_SHADER_RESOURCE_VIEW_DESC k_depthBufferSRVTemplate {
		DXGI_FORMAT_UNKNOWN,
		D3D12_SRV_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		{.Texture2D = D3D12_TEX2D_SRV{
			0,		// MostDetailedMip
			1,		// MipLevels;
			0,		// PlaneSlice;
			0.f		// ResourceMinLODClamp;
		}}
	};

	constexpr D3D12_DEPTH_STENCIL_VIEW_DESC k_depthStencilViewTemplate{
		DXGI_FORMAT_UNKNOWN,
		D3D12_DSV_DIMENSION_TEXTURE2D,
		D3D12_DSV_FLAG_NONE,
		{.Texture2D = D3D12_TEX2D_DSV{
			0	//MipSlice
		}}
	};

	constexpr DXGI_SWAP_CHAIN_DESC1 k_swapChainTemplate{
		0,	//Width;
		0,	//Height;
		DXGI_FORMAT_UNKNOWN,
		false,
		DXGI_SAMPLE_DESC{
			1,	//Count
			0	//Quality
		},
		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		0, //BufferCount
		DXGI_SCALING_STRETCH,
		DXGI_SWAP_EFFECT_FLIP_DISCARD,
		DXGI_ALPHA_MODE_UNSPECIFIED,
		0	//Flags
	};

	constexpr D3D12_RENDER_TARGET_VIEW_DESC k_renderTargetViewTemplate{
		DXGI_FORMAT_UNKNOWN,
		D3D12_RTV_DIMENSION_TEXTURE2D,
		{ .Texture2D = D3D12_TEX2D_RTV{} }
	};

	constexpr D3D12_RESOURCE_DESC k_GPassMainTextureTemplate{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,	// Alignment;
		0,	// Width;
		0,	// Height;
		1,	// DepthOrArraySize;
		0,	// MipLevels;
		DXGI_FORMAT_R16G16B16A16_UNORM,
		DXGI_SAMPLE_DESC{
			1,	//Count
			0	//Quality
		},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	};

	constexpr D3D12_RESOURCE_DESC k_GPassDepthTextureTemplate{
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,	// Alignment;
		0,	// Width;
		0,	// Height;
		1,	// DepthOrArraySize;
		1,	// MipLevels;
		DXGI_FORMAT_D32_FLOAT,
		DXGI_SAMPLE_DESC{
			1,	//Count
			0	//Quality
		},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};
}