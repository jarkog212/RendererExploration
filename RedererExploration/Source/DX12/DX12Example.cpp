#include "DX12Utils.h"
#include "DX12DescriptorHeap.h"
#include "DX12Renderer.h"
#include "DX12CommonDescriptors.h"

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> COMRenderRootSignature = nullptr;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> COMRenderPipelineState = nullptr;
} // Example

ID3D12RootSignature* GetRootSignature()
{
	if (::COMRenderRootSignature) [[likely]]
		return ::COMRenderRootSignature.Get();

		// Create signature = memory layout for buffers
		DX12::RootParameter rootParams[2]{};
		DX12::DescriptorRange descriptorRanges[]{
			DX12::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, DX12::GetSRVHeap()->GetCapacity(), 0) };

		rootParams[0].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, descriptorRanges, 1);

		rootParams[1].AsConstants(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);

		D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
		desc.NumParameters = _countof(rootParams);
		desc.pParameters = rootParams;

		::COMRenderRootSignature = DX12::Helpers::CreateRootSignature(desc);

		return ::COMRenderRootSignature.Get();
}

ID3D12PipelineState* GetPipelineState()
{
	if (::COMRenderPipelineState) [[likely]]
		return ::COMRenderPipelineState.Get();

		using namespace DX12::PipelineStream;
		struct {
			SubRootSignature rs{ GetRootSignature() };
		}stream;

		::COMRenderPipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

		return ::COMRenderPipelineState.Get();
}

ID3D12GraphicsCommandList6* ExampleRenderBuild()
{
	DX12::DescriptorHeap* srvHeap = DX12::GetSRVHeap();

	ID3D12GraphicsCommandList6* commandList = DX12::GetMainRendererCommand()->GetCommandList();

	commandList->SetGraphicsRootSignature(GetRootSignature());
	commandList->SetPipelineState(GetPipelineState());

	ID3D12DescriptorHeap* heaps[]{ srvHeap->GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetGraphicsRootDescriptorTable(0, srvHeap->GetGpuPtr());

	commandList->SetGraphicsRoot32BitConstant(1, (int)Utils::GetFPS(), 0);
	commandList->SetGraphicsRoot32BitConstant(1, 42, 1);

	//commandList->SetGraphicsRootConstantBufferView(1,);

	return commandList;
}

