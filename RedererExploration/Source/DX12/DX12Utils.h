#pragma once

#include "DX12Core.h"
#include "DX12CommonDescriptors.h"
#include "Utils.h"

namespace DX12::Helpers
{
	inline DXGI_FORMAT ConvertDepthDSVtoSRVFormat(DXGI_FORMAT dsv)
	{
		switch (dsv)
		{
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_D16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	inline DXGI_FORMAT ConvertStencilDSVtoSRVFormat(DXGI_FORMAT dsv)
	{
		switch (dsv)
		{
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	// Max mem 64 DWORDS:
	// descriptor table = 1 DWORD
	// root constants = 1 DWORD (per constant)
	inline ID3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& desc)
	{
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
		versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedDesc.Desc_1_1 = desc;

		Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

#ifdef _DEBUG
		HRESULT_CHECK_FALLBACK(
			D3D12SerializeVersionedRootSignature(&versionedDesc, &rootSignatureBlob, &errorBlob),
			const char* errorMsg = { errorBlob ? (const char*)errorBlob->GetBufferPointer() : "" },
			assert(false),
			"Root signature creation fail : %s",
			errorMsg);
#else
		HRESULT_CHECK_FALLBACK(
			D3D12SerializeVersionedRootSignature(&versionedDesc, &rootSignatureBlob, &errorBlob), ,
			assert(false),
			"Root signature creation fail.");
#endif

		ID3D12RootSignature* signature = nullptr;
		HRESULT_CHECK_FALLBACK(
			DX12::GetRendererDevice()->CreateRootSignature(
				0,
				rootSignatureBlob->GetBufferPointer(),
				rootSignatureBlob->GetBufferSize(),
				IID_PPV_ARGS(&signature)), ,
			Utils::COM::SafeRelease(signature),
			"Could not create root signature.");

		return signature;
	}

	inline ID3D12PipelineState* CreatePipelineState(void* stream, unsigned long long int streamSize)
	{
		CRITICAL_ASSERT(stream && streamSize > 0, "Cannot create pipeline state without valid stream.");

		D3D12_PIPELINE_STATE_STREAM_DESC desc{};
		desc.pPipelineStateSubobjectStream = stream;
		desc.SizeInBytes = streamSize;

		ID3D12PipelineState* state = nullptr;
		HRESULT_CHECK_FALLBACK(
			DX12::GetRendererDevice()->CreatePipelineState(&desc, IID_PPV_ARGS(&state)), ,
			assert(false),
			"Could not create pipeline state.");

		return state;
	}
}