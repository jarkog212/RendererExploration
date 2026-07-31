#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace DX12
{
	static constexpr unsigned int k_frameBuffers = 3;

	static D3D_FEATURE_LEVEL g_expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

	ID3D12Device8* GetRendererDevice();

	IDXGIFactory7* GetDxgiFactory();

	IDXGIAdapter4* GetDxgiAdapter();

	void ResetAll();

	class DX12Command
	{
		explicit DX12Command(D3D12_COMMAND_LIST_TYPE type);

		void PreRender();

		void PostRender();

	private:

		void WaitForGPU() {};

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> _commandList = nullptr;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocators[k_frameBuffers];
		unsigned int _currentFrame = 0;
	};

	class DX12Renderer
	{
	public:

		static DX12Renderer& Get()
		{
			static DX12Renderer instance;
			return instance;
		}

		void Render();

	private:

		DX12Renderer() = default;
		~DX12Renderer() {};

		void PreRender();
		void PostRender() {};

	};
};