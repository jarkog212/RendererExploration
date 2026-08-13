#pragma once

#include "DX12Resources.h"
#include "DX12RenderTargetTexture.h"
#include "DX12DepthBufferTexture.h"
#include "DX12Math.h"

namespace 
{
	static const DX12::Math::Vector2u32 k_defaultDimensions = { 100,100 };
}

namespace DX12
{
	namespace GPassSpace
	{
		void WarmUp();
		void Release();

		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();
	}

	class GPass : public RenderPassBase
	{
	public:

		static GPass Create(DX12::Math::Vector2u32& size);

		GPass(DX12::RenderTargetTexture&& mainBuffer, DX12::DepthBufferTexture&& depthBuffer);
		GPass(GPass&& other) noexcept;
		GPass& operator=(GPass&& other) noexcept;

		NO_COPY(GPass);

		void RenderPass(ID3D12GraphicsCommandList6* commandList) final override;

		void SetDimensions(DX12::Math::Vector2u32 dimensions);

		DX12::RenderTargetTexture _mainBuffer;
		DX12::DepthBufferTexture _depthBuffer;
		DX12::Math::Vector2u32 _dimensions = ::k_defaultDimensions;

	private:

		static DX12::RenderTargetTexture CreateMainBufferTemplate(DX12::Math::Vector2u32& size);
		static DX12::DepthBufferTexture CreateDepthBufferTemplate(DX12::Math::Vector2u32& size);

		void DepthPass(ID3D12GraphicsCommandList6* commandList);
		void FullPass(ID3D12GraphicsCommandList6* commandList);
	};
}