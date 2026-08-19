#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#include <filesystem>

namespace DX12::CC
{
	constexpr int k_forAllFrames = -1;
	constexpr unsigned int k_commandFrameBuffers = 3;
	constexpr unsigned int k_swapChainBuffers = 3;
	constexpr unsigned int k_maxResourceBarriers = 32;
	constexpr DXGI_FORMAT k_defaultRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;


#ifdef _DEBUG
	constexpr float k_clearValue[4]{ 0.5f, 0.5f, 0.5f, 1.f };
#else
	constexpr float k_clearValue[4]{};
#endif

	namespace RootSignature
	{
		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_LegacyStagesOnly =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_ModernStagesOnly =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_VertexPixelStagesOnly =
			DX12::CC::RootSignature::k_LegacyStagesOnly
			| DX12::CC::RootSignature::k_ModernStagesOnly;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_PixelStageOnly =
			DX12::CC::RootSignature::k_VertexPixelStagesOnly
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	} // RootSignatureSpec
}