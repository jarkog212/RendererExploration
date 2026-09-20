#pragma once

#include "DX12Resources.h"
#include "DX12RenderTargetTexture.h"
#include "DX12DepthBufferTexture.h"
#include "DX12CommonDescriptors.h"

#include "DirectXMath.h"

namespace 
{
	static const DirectX::XMUINT2 k_defaultDimensions = { 100,100 };
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

		DepthPass* GetDepthPass(DirectX::XMUINT2 size = ::k_defaultDimensions);
		GPassRT* GetGPassRTPass(DirectX::XMUINT2 size = ::k_defaultDimensions);

		RenderPassBase* GetHeadNode();
		RenderPassBase* GetTailNode();

		void SetDimensions(const DirectX::XMUINT2& size);

		struct PSOBase
		{
			PipelineStream::SubRootSignature _rs{ DX12::GPassSpace::GetRootSignature() };
			PipelineStream::SubPrimitiveTopology _primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
			PipelineStream::SubRenderTargetFormats _renderTargetFormats{
				{
					{DX12::CC::Desc::GPassRes::k_mainTextureTemplate.Format},
					1
				}};
			PipelineStream::SubDepthStencilFormat _depthStencilFormat{ DX12::CC::Desc::GPassRes::k_depthTextureTemplate.Format };
			PipelineStream::SubRasterizer _rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
			PipelineStream::SubDepthStencil1 _depth{ DX12::CC::Desc::DepthStencil::k_disabledBothTemplate };
		};
	}

	class DepthPass : public RenderPassBase
	{
	public:

		static DepthPass Create(const DirectX::XMUINT2& size);

		DepthPass(DX12::DepthBufferTexture&& depthBuffer);
		DEBUG_DESTRUCTOR(DepthPass);
		DepthPass(DepthPass&& other) noexcept;
		DepthPass& operator=(DepthPass&& other) noexcept;

		NO_COPY(DepthPass);

		void PrePassResourceBarriers() final override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) final override;
		void PostPassResourceBarriers() final override;

		const DX12::DepthBufferTexture* GetDepthBuffer() const { return &_depthBuffer; }
		void SetDimensions(const DirectX::XMUINT2& dimensions);

	private:

		DX12::DepthBufferTexture _depthBuffer;
		DirectX::XMUINT2 _dimensions = ::k_defaultDimensions;

		static DX12::DepthBufferTexture CreateDepthBuffer(const DirectX::XMUINT2& size);
	};

	class GPassRT : public RenderPassBase
	{
	public:

		static GPassRT Create(const DirectX::XMUINT2& size);

		GPassRT(DX12::RenderTargetTexture&& mainBuffer);
		DEBUG_DESTRUCTOR(GPassRT);
		GPassRT(GPassRT&& other) noexcept;
		GPassRT& operator=(GPassRT&& other) noexcept;

		NO_COPY(GPassRT);

		void PrePassResourceBarriers() final override;
		void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) final override;
		void PostPassResourceBarriers() final override;

		void LinkDepthPass(const DX12::DepthPass* pass) { _linkToDepthPass = pass; }
		void UnlinkDepthPass() { _linkToDepthPass = nullptr; }
		void SetDimensions(const DirectX::XMUINT2& dimensions);
		DX12::DescriptorHandleView GetMainBufferHandleView() const { return _mainBuffer.GetSRVHandle(); }

	private:

		const DX12::DepthPass* _linkToDepthPass = nullptr; //non-owning
		DX12::RenderTargetTexture _mainBuffer;
		DirectX::XMUINT2 _dimensions = ::k_defaultDimensions;

		static DX12::RenderTargetTexture CreateRenderTarget(const DirectX::XMUINT2& size);
	};
}