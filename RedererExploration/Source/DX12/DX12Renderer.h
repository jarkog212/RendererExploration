#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#include "Utils.h"

namespace DX12
{
	static constexpr unsigned int k_frameBuffers = 3;
	static constexpr int k_allPending = -1;
	static D3D_FEATURE_LEVEL g_expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

	ID3D12Device8* GetRendererDevice();
	IDXGIFactory7* GetDxgiFactory();
	IDXGIAdapter4* GetDxgiAdapter();
	class DX12Command* GetMainRenderCommand();
	class DX12DescriptorHeap* GetRTVHeap();
	class DX12DescriptorHeap* GetDSVHeap();
	class DX12DescriptorHeap* GetSRVHeap();
	class DX12DescriptorHeap* GetUAVHeap();

	void SetPendingResourceFreeingFlag();
	bool TestPendingResourceFreeingFlag(const unsigned int frame);

	bool Initialize();
	void Render();
	void Cleanup();

	class DX12Command
	{
	public:

		explicit DX12Command(D3D12_COMMAND_LIST_TYPE type);
		~DX12Command() { Release(); };

		NO_COPY(DX12Command);
		NO_MOVE(DX12Command);

		void PreRender();
		void PostRender();

		ID3D12CommandQueue* GetCommandQueue() const { return _commandQueue.Get(); };
		ID3D12GraphicsCommandList6* GetCommandList() const { return _commandList.Get(); };
		constexpr unsigned int GetCurrentFrame() const { return _currentFrame; };

		void Release();

	private:

		void Flush();

		class DX12CommandFrame
		{
		public:

			DX12CommandFrame() = default;

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
		DX12CommandFrame _frame[k_frameBuffers];
		Microsoft::WRL::ComPtr<ID3D12Fence1> _fence = nullptr;
		HANDLE _fenceEvent = 0;
		unsigned int _currentFrame = 0;
		unsigned int _issuableFenceValue = 0;
	};
};