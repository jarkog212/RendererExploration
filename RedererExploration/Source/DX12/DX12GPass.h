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
	class GPass : public IRenderPass
	{
	public:

		static GPass Create(DX12::Math::Vector2u32& size);

		GPass(DX12::TextureSpecs& mainBufferSpecs, DX12::TextureSpecs& depthBufferSpecs);
		~GPass() { Release(); }
		GPass(GPass&& other) noexcept;
		GPass& operator=(GPass&& other) noexcept;

		NO_COPY(GPass);

		void Initialize() final override {};
		void Release() final override {};
		void SetNext(IRenderPass* pass) final override {};
		void Render() final override {};

		void SetDimensions(DX12::Math::Vector2u32 dimensions) { _dimensions = dimensions; }

		DX12::RenderTargetTexture _mainBuffer;
		DX12::DepthBuffer _depthBuffer;
		DX12::Math::Vector2u32 _dimensions = ::k_defaultDimensions;
	};
}