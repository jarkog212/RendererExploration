#pragma once

#include "DX12Resources.h"
#include "DX12RenderTarget.h"
#include "DX12DepthBuffer.h"
#include "DX12Math.h"

namespace 
{
	static const DX12::Math::Vector2u32 k_defaultDimensions = { 100,100 };
}

namespace DX12
{
	class GPass : public RenderPassBase
	{
	public:

		static GPass Create(DX12::Math::Vector2u32& size);

		GPass(DX12::RenderTargetTexture&& mainBuffer, DX12::DepthBuffer&& depthBuffer);
		GPass(GPass&& other) noexcept;
		GPass& operator=(GPass&& other) noexcept;

		NO_COPY(GPass);

		void RenderPass(DX12::GfxCommandExecutor& commandExecutor) final override;

		void SetDimensions(DX12::Math::Vector2u32 dimensions);

		DX12::RenderTargetTexture _mainBuffer;
		DX12::DepthBuffer _depthBuffer;
		DX12::Math::Vector2u32 _dimensions = ::k_defaultDimensions;

	private:

		static DX12::RenderTargetTexture CreateMainBufferTemplate(DX12::Math::Vector2u32& size);
		static DX12::DepthBuffer CreateDepthBufferTemplate(DX12::Math::Vector2u32& size);
	};
}