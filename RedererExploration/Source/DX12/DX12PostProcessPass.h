#pragma once

#include "DX12Resources.h"

namespace DX12
{
	namespace PostProcessSpace
	{
		void WarmUp();
		void Release();

		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();
	}

	class PostProcessPass : public RenderPassBase
	{
		PostProcessPass();
		PostProcessPass(PostProcessPass&& other) noexcept;
		PostProcessPass& operator=(PostProcessPass&& other) noexcept;

		NO_COPY(PostProcessPass);

		void PrePassResourceBarriers() override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) override;
		void PostPassResourceBarriers() override;
	};
}
