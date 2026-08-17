#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class PostProcessPass;

	namespace PostProcessSpace
	{
		void WarmUp();
		void Release();

		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();

		PostProcessPass* GetPostProcessPass();
	}

	class PostProcessPass : public RenderPassBase
	{
	public:

		PostProcessPass() = default;
		PostProcessPass(PostProcessPass&& other) noexcept;
		PostProcessPass& operator=(PostProcessPass&& other) noexcept;

		NO_COPY(PostProcessPass);

		void PrePassResourceBarriers() override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) override;
		void PostPassResourceBarriers() override;

		void LinkRenderTargetRTV(DX12::DescriptorHandleView rtv) { _renderTagetRTV = rtv; }
		void UnlinkRenderTargetRTV() { _renderTagetRTV = {}; }
		void LinkInputSRV(DX12::DescriptorHandleView srvView) { _inputSRV = srvView; }
		void UnlinkInputRTV() { _inputSRV = {}; }

	private:

		DX12::DescriptorHandleView _renderTagetRTV{};
		DX12::DescriptorHandleView _inputSRV{};
	};
}
