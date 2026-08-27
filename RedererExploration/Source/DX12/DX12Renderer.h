#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"

namespace DX12
{
	class RendererWorker
	{
	public:

		RendererWorker(D3D12_COMMAND_LIST_TYPE type);
		~RendererWorker() { DEBUG_DESTRUCTOR_BODY(RendererWorker) Release(); };

		NO_COPY(RendererWorker);
		NO_MOVE(RendererWorker);

		void PreRender();
		void PostRender();

		ID3D12CommandQueue* GetCommandQueue() const { return _commandQueue.Get(); };
		ID3D12GraphicsCommandList6* GetCommandList() const { return _commandList.Get(); };
		constexpr unsigned int GetCurrentFrame() const { return _currentFrame; };
		constexpr void LinkRenderSurface(DX12::RenderSurface* surface) { _linkedSurface = surface; }
		constexpr void UnlinkSurface() { _linkedSurface = nullptr; }
		constexpr DX12::RenderSurface* GetLinkedSurface() const { return _linkedSurface; }

		void UpdateSurfaceViewport();
		void ResizeSurface();
		void Release();
		void Flush();

	private:

		class RendererWorkerFrame
		{
		public:

			RendererWorkerFrame() = default;
			~RendererWorkerFrame() { DEBUG_DESTRUCTOR_BODY(RendererWorkerFrame) Release(); }

			NO_COPY(RendererWorkerFrame);
			NO_MOVE(RendererWorkerFrame);

			bool Setup(D3D12_COMMAND_LIST_TYPE type, int index = -1);
			void Release();
			void Reinstate();

			void WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence);

			void IssueFenceValue(int value) { _fenceValue = value; }
			ID3D12CommandAllocator* GetAllocator() { return _allocator.Get(); }

		private:

			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _allocator = nullptr;
			unsigned int _fenceValue = 0;
		};

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> _commandList = nullptr;
		RendererWorkerFrame _frames[DX12::CC::k_rendererWorkerFrameBuffers];
		Microsoft::WRL::ComPtr<ID3D12Fence1> _fence = nullptr;
		HANDLE _fenceEvent = 0;
		unsigned int _currentFrame = 0;
		unsigned int _issuableFenceValue = 0;
		DX12::RenderSurface* _linkedSurface = nullptr; //non-owning
	};
};