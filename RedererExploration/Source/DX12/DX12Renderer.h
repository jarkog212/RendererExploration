#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"

namespace DX12
{
	class RendererCommand
	{
	public:

		RendererCommand(D3D12_COMMAND_LIST_TYPE type);
		~RendererCommand() { DEBUG_DESTRUCTOR_BODY(RendererCommand) Release(); };

		NO_COPY(RendererCommand);
		NO_MOVE(RendererCommand);

		void PreRender();
		void PostRender();

		ID3D12CommandQueue* GetCommandQueue() const { return _commandQueue.Get(); };
		ID3D12GraphicsCommandList6* GetCommandList() const { return _commandList.Get(); };
		constexpr unsigned int GetCurrentFrame() const { return _currentFrame; };
		constexpr void LinkRenderSurface(DX12::RenderSurface* surface) { _linkedSurface = surface; }
		constexpr void UnlinkSurface() { _linkedSurface = nullptr; }
		constexpr DX12::RenderSurface* GetLinkedSurface() const { return _linkedSurface; }

		void UpdateSurfaceViewport();
		void Release();
		void Flush();

	private:

		class GfxCommandFrame
		{
		public:

			GfxCommandFrame() = default;
			~GfxCommandFrame() { DEBUG_DESTRUCTOR_BODY(GfxCommandFrame) Reset(); }

			NO_COPY(GfxCommandFrame);
			NO_MOVE(GfxCommandFrame);

			bool Setup(D3D12_COMMAND_LIST_TYPE type, int index = -1);
			void Reset();

			void WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence);

			void IssueFenceValue(int value) { _fenceValue = value; };
			ID3D12CommandAllocator* GetAllocator() { return _allocator.Get(); };

		private:

			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _allocator = nullptr;
			unsigned int _fenceValue = 0;
		};

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> _commandList = nullptr;
		GfxCommandFrame _frame[DX12::CC::k_commandFrameBuffers];
		Microsoft::WRL::ComPtr<ID3D12Fence1> _fence = nullptr;
		HANDLE _fenceEvent = 0;
		unsigned int _currentFrame = 0;
		unsigned int _issuableFenceValue = 0;
		DX12::RenderSurface* _linkedSurface = nullptr; //non-owning
	};
};