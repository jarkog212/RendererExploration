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
	class DepthPass;
	class GPassRT;

	namespace GPassSpace
	{
		void WarmUp();
		void Release();

		ID3D12RootSignature* GetRootSignature();
		ID3D12PipelineState* GetPipelineState();

		DepthPass* GetDepthPass(DX12::Math::Vector2u32 size = { 0, 0 });
		GPassRT* GetGPassRTPass(DX12::Math::Vector2u32 size = { 0, 0 });

		RenderPassBase* GetHeadNode();
		RenderPassBase* GetTailNode();

		void SetDimensions(const DX12::Math::Vector2u32& size);
	}

	class DepthPass : public RenderPassBase
	{
	public:

		static DepthPass Create(const DX12::Math::Vector2u32& size);

		DepthPass(DX12::DepthBufferTexture&& depthBuffer);
		DepthPass(DepthPass&& other) noexcept;
		DepthPass& operator=(DepthPass&& other) noexcept;

		NO_COPY(DepthPass);

		void PrePassResourceBarriers() override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) override;
		void PostPassResourceBarriers() override;

		const DX12::DepthBufferTexture* GetDepthBuffer() const { return &_depthBuffer; }
		void SetDimensions(const DX12::Math::Vector2u32& dimensions);

	private:

		DX12::DepthBufferTexture _depthBuffer;
		DX12::Math::Vector2u32 _dimensions = ::k_defaultDimensions;

		static DX12::DepthBufferTexture CreateDepthBuffer(const DX12::Math::Vector2u32& size);
	};

	class GPassRT : public RenderPassBase
	{
	public:

		static GPassRT Create(const DX12::Math::Vector2u32& size);

		GPassRT(DX12::RenderTargetTexture&& mainBuffer);
		GPassRT(GPassRT&& other) noexcept;
		GPassRT& operator=(GPassRT&& other) noexcept;

		NO_COPY(GPassRT);

		void PrePassResourceBarriers() override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) override;
		void PostPassResourceBarriers() override;

		void LinkDepthPass(const DX12::DepthPass* pass) { _linkToDepthPass = pass; }
		void UnlinkDepthPass() { _linkToDepthPass = nullptr; }
		void SetDimensions(const DX12::Math::Vector2u32& dimensions);
		DX12::DescriptorHandleView GetMainBufferHandleView() const { return _mainBuffer.GetSRVHandle(); }

	private:

		const DX12::DepthPass* _linkToDepthPass = nullptr; //non-owning
		DX12::RenderTargetTexture _mainBuffer;
		DX12::Math::Vector2u32 _dimensions = ::k_defaultDimensions;

		static DX12::RenderTargetTexture CreateRenderTarget(const DX12::Math::Vector2u32& size);
	};
}