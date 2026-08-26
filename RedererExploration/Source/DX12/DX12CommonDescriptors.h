#pragma once

#include "DX12CommonConstants.h"

namespace DX12::CC::Desc
{
	namespace Heap
	{
		constexpr D3D12_HEAP_PROPERTIES k_baseTemplate {
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,	// CreationNodeMask
			0	// VisibleNodeMask
		};

		constexpr D3D12_HEAP_PROPERTIES k_uploadTemplate {
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,	// CreationNodeMask
			0	// VisibleNodeMask
		};
	} // Heap

	namespace RTT
	{
		constexpr D3D12_SHADER_RESOURCE_VIEW_DESC k_SRVTemplate {
			DXGI_FORMAT_UNKNOWN,
			D3D12_SRV_DIMENSION_TEXTURE2D,
			D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			{.Texture2D = D3D12_TEX2D_SRV{
				0,		// MostDetailedMip
				1,		// MipLevels
				0,		// PlaneSlice
				0.f		// ResourceMinLODClamp
			}}
		};
	} // RTT

	namespace DepthStencil
	{
		constexpr D3D12_SHADER_RESOURCE_VIEW_DESC k_SRVTemplate {
			DXGI_FORMAT_UNKNOWN,
			D3D12_SRV_DIMENSION_TEXTURE2D,
			D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			{.Texture2D = D3D12_TEX2D_SRV{
				0,		// MostDetailedMip
				1,		// MipLevels
				0,		// PlaneSlice
				0.f		// ResourceMinLODClamp
			}}
		};

		constexpr D3D12_DEPTH_STENCIL_VIEW_DESC k_viewTemplate {
			DXGI_FORMAT_UNKNOWN,
			D3D12_DSV_DIMENSION_TEXTURE2D,
			D3D12_DSV_FLAG_NONE,
			{.Texture2D = D3D12_TEX2D_DSV{
				0	//MipSlice
			}}
		};

		constexpr D3D12_DEPTH_STENCIL_DESC1 k_disabledBothTemplate{
			false, // DepthEnable
			D3D12_DEPTH_WRITE_MASK_ZERO,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			false, // StencilEnable
			0, // StencilReadMask
			0, // StencilWriteMask
			D3D12_DEPTH_STENCILOP_DESC{}, // FrontFace
			D3D12_DEPTH_STENCILOP_DESC{}, // BackFace
			false // DepthBoundsTestEnable
		};
	} // DepthStencil

	namespace SwapChain
	{
		constexpr DXGI_SWAP_CHAIN_DESC1 k_descTemplate {
			0,	//Width
			0,	//Height
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
	} // SwapChain

	namespace RTView
	{
		constexpr D3D12_RENDER_TARGET_VIEW_DESC k_descTemplate {
			DXGI_FORMAT_UNKNOWN,
			D3D12_RTV_DIMENSION_TEXTURE2D,
			{.Texture2D = D3D12_TEX2D_RTV{} }
		};
	} // RTView

	namespace GPassRes
	{
		constexpr D3D12_RESOURCE_DESC k_mainTextureTemplate {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,	// Alignment
			0,	// Width
			0,	// Height
			1,	// DepthOrArraySize
			0,	// MipLevels
			DXGI_FORMAT_R16G16B16A16_UNORM,
			DXGI_SAMPLE_DESC{
				1,	//Count
				0	//Quality
			},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		};

		constexpr D3D12_RESOURCE_DESC k_depthTextureTemplate {
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,	// Alignment
			0,	// Width
			0,	// Height
			1,	// DepthOrArraySize
			1,	// MipLevels
			DXGI_FORMAT_D32_FLOAT,
			DXGI_SAMPLE_DESC{
				1,	//Count
				0	//Quality
			},
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		};
	} // GPassRes

	namespace Rasterizer
	{
		constexpr D3D12_RASTERIZER_DESC k_noCullTemplate {
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_NONE,
			false, // FrontCounterClockwise
			0, // DepthBias
			0, // DepthBiasClamp
			0, // SlopeScaledDepthBias
			true, // DepthClipEnable
			true, // MultisampleEnable
			false,// AntialiasedLineEnable;
			0, // ForcedSampleCount;
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		constexpr D3D12_RASTERIZER_DESC k_backCullTemplate {
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_BACK,
			false, // FrontCounterClockwise
			0, // DepthBias
			0, // DepthBiasClamp
			0, // SlopeScaledDepthBias
			true, // DepthClipEnable
			true, // MultisampleEnable
			false,// AntialiasedLineEnable;
			0, // ForcedSampleCount;
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		constexpr D3D12_RASTERIZER_DESC k_frontCullTemplate {
			D3D12_FILL_MODE_SOLID,
			D3D12_CULL_MODE_FRONT,
			false, // FrontCounterClockwise
			0, // DepthBias
			0, // DepthBiasClamp
			0, // SlopeScaledDepthBias
			true, // DepthClipEnable
			true, // MultisampleEnable
			false,// AntialiasedLineEnable;
			0, // ForcedSampleCount;
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		constexpr D3D12_RASTERIZER_DESC k_wireframeTemplate {
			D3D12_FILL_MODE_WIREFRAME,
			D3D12_CULL_MODE_NONE,
			false, // FrontCounterClockwise
			0, // DepthBias
			0, // DepthBiasClamp
			0, // SlopeScaledDepthBias
			true, // DepthClipEnable
			true, // MultisampleEnable
			false,// AntialiasedLineEnable;
			0, // ForcedSampleCount;
			D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};
	} // Rasterizer

	namespace RootSignature
	{
		constexpr D3D12_ROOT_SIGNATURE_DESC1 k_vertexPixelTemplate {
			0, // NumParameters
			nullptr, // pParameters
			0, // NumStaticSamplers
			nullptr, // pStaticSamplers
			DX12::CC::RootSignature::k_VertexPixelStagesOnly
		};
	} // RootSignature 

	namespace BufferResource
	{
		constexpr D3D12_RESOURCE_DESC k_bufferTemplate {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,	// Alignment
			0,	// Width
			1,	// Height
			1,	// DepthOrArraySize
			1,	// MipLevels
			DXGI_FORMAT_UNKNOWN,
			DXGI_SAMPLE_DESC{
				1,	//Count
				0	//Quality
			},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE
		};
	}
}