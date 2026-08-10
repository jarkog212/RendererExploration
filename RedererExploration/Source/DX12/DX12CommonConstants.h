#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>

#include <filesystem>

namespace DX12::CC
{
	constexpr int k_forAllFrames = -1;
	constexpr unsigned int k_frameBuffers = 3;
	constexpr DXGI_FORMAT k_defaultRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static std::filesystem::path k_shaderFilePath(".\\Shaders");
	static std::filesystem::path k_precompiledShaderFilePath = ".\\Shaders\\shaders.bin";

	namespace RootSignatureSpec
	{
		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_LegacyStagesOnly =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_ModernStagesOnly =
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_VertexPixelStagesOnly =
			DX12::CC::RootSignatureSpec::k_LegacyStagesOnly
			| DX12::CC::RootSignatureSpec::k_ModernStagesOnly;

		constexpr D3D12_ROOT_SIGNATURE_FLAGS k_PixelStageOnly =
			DX12::CC::RootSignatureSpec::k_VertexPixelStagesOnly
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	}
}