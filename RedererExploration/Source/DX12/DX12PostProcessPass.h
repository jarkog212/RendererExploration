#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class PostProcessPass;
	class GPassRT;

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
		DEBUG_DESTRUCTOR(PostProcessPass);
		PostProcessPass(PostProcessPass&& other) noexcept;
		PostProcessPass& operator=(PostProcessPass&& other) noexcept;

		NO_COPY(PostProcessPass);

		void PrePassResourceBarriers() final override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) final override;
		void PostPassResourceBarriers() final override;

		void LinkRenderSurface(DX12::RenderSurface& surface) { _renderSurface = &surface; }
		void UnlinkRenderSurface() { _renderSurface = nullptr; }
		void LinkInputGPass(DX12::GPassRT* gPass) { _inputGPass = gPass; }
		void UnlinkInputGPass() { _inputGPass = nullptr; }

	private:

		const DX12::RenderSurface* _renderSurface = nullptr; //non-owning
		const DX12::GPassRT* _inputGPass = nullptr; // non-owning
	};
}
