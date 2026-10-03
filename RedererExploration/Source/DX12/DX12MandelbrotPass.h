#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class MandelbrotPass;
	namespace Shaders
	{
		class Shader;
	}

	namespace MandelbrotSpace
	{
		void WarmUp();
		void Release();

		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();
		DX12::Shaders::Shader* GetMandelbrotPixelShader();

		MandelbrotPass* GetMandelbrotPass();
	}

	class MandelbrotPass : public RenderPassBase
	{
	public:

		MandelbrotPass() = default;
		DEBUG_DESTRUCTOR(MandelbrotPass);
		MandelbrotPass(MandelbrotPass&& other) noexcept;
		MandelbrotPass& operator=(MandelbrotPass&& other) noexcept;

		NO_COPY(MandelbrotPass);

		void PrePassResourceBarriers() final override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) final override;
		void PostPassResourceBarriers() final override;

		void LinkRenderSurface(DX12::RenderSurface& surface) { _renderSurface = &surface; }
		void UnlinkRenderSurface() { _renderSurface = nullptr; }

	private:

		const DX12::RenderSurface* _renderSurface = nullptr; //non-owning
	};
}