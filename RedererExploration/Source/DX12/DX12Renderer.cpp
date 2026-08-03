#include "DX12Renderer.h"
#include "DX12Resources.h"

#include <cstdio>
#include <memory>
#include <mutex>

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12Device8> COMRendererDevice = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIFactory7> COMDxgiFactory = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIAdapter4> COMDxgiAdapter = nullptr;

	static int DeferredResourceFreeingFlag = 0;
	static_assert(sizeof(DeferredResourceFreeingFlag) * sizeof(char) > DX12::k_frameBuffers, "Flag bit-buffer needs to be wide enough for frame buffer count.");

	static void PendingResourceFreeingRoutine() 
	{
		static std::mutex mutex;
		
		if (!(bool)(::DeferredResourceFreeingFlag & (1ull << DX12::GetDx12MainRenderCommand()->GetCurrentFrame())))
			return;
		
		std::lock_guard lock(mutex);

		::DeferredResourceFreeingFlag &= ~(1ull << DX12::GetDx12MainRenderCommand()->GetCurrentFrame());
	};
}

ID3D12Device8* DX12::GetRendererDevice()
{
	ID3D12Device8* device = ::COMRendererDevice.Get();
	if (device)
		return device;

	HRESULT_CHECKED(
		D3D12CreateDevice(GetDxgiAdapter(), g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECKED_FALLBACK(device->SetName(L"MainDevice"),, "Could not set Dx12 device's name.");

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECKED(device->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather queue interface.");
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif

	::COMRendererDevice.Attach(device);
	return device;
}

IDXGIFactory7* DX12::GetDxgiFactory()
{	
	IDXGIFactory7* factory = ::COMDxgiFactory.Get();
	if (factory)
		return factory;

	unsigned int flags = 0;

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12Debug6> debugInterface = nullptr;
		HRESULT_CHECKED(D3D12GetDebugInterface(__uuidof(ID3D12Debug6), (void**)debugInterface.GetAddressOf()), "Failed to get debug interface.");
		debugInterface->EnableDebugLayer();

		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT_CHECKED(CreateDXGIFactory2(flags, __uuidof(IDXGIFactory7), (void**)&factory), "DXGI Factory issue.");

	::COMDxgiFactory.Attach(factory);
	return factory;
}

IDXGIAdapter4* DX12::GetDxgiAdapter()
{
	IDXGIAdapter4* adapter = ::COMDxgiAdapter.Get();
	if (adapter)
		return adapter;

	IDXGIFactory7* factory = GetDxgiFactory();
	for (unsigned int i = 0;
		factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter4), (void**)&adapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter, g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		else
		{
			Utils::COM::SafeRelease(adapter);
		}
	}

	if (!adapter)
	{
		DISPLAY_ERROR(L"No good GPU adapter.");
		return nullptr;
	}

	constexpr D3D_FEATURE_LEVEL featureLevels[4]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsData;
	featureLevelsData.NumFeatureLevels = _countof(featureLevels);
	featureLevelsData.pFeatureLevelsRequested = featureLevels;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT_CHECKED(
		D3D12CreateDevice(adapter, g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device.GetAddressOf()),
		"Could not create Dx12 device with selected adapter.");
	
	HRESULT_CHECKED(
		device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData)),
		"Could not access Dx12 device's feature support information.");

	if (featureLevelsData.MaxSupportedFeatureLevel < g_expectedMinimumFeatureLevel)
	{
		DISPLAY_ERROR(L"Dx12 device's feature level support is lover than minimum requirement.");
		Utils::COM::SafeRelease(adapter);
		return nullptr;
	}

	g_expectedMinimumFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
	::COMDxgiAdapter.Attach(adapter);
	return adapter;
}

DX12::DX12Command* DX12::GetDx12MainRenderCommand()
{
	static DX12::DX12Command instance(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return &instance;
}

DX12::DX12DescriptorHeap* DX12::GetRTVHeap()
{
	static DX12::DX12DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return &instance;
}

DX12::DX12DescriptorHeap* DX12::GetDSVHeap()
{
	static DX12::DX12DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return &instance;
}

DX12::DX12DescriptorHeap* DX12::GetSRVHeap()
{
	static DX12::DX12DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return &instance;
}

DX12::DX12DescriptorHeap* DX12::GetUAVHeap()
{
	static DX12::DX12DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return &instance;
}

void DX12::SetPendingResourceFreeingFlag()
{
	::DeferredResourceFreeingFlag |= 1ull << DX12::GetDx12MainRenderCommand()->GetCurrentFrame();
}

void DX12::Initialize()
{
	bool result = true;

	DX12::DX12DescriptorHeap* heap = GetRTVHeap();
	result |= heap->Setup(512, false);
	HRESULT_CHECKED_FALLBACK(heap->GetHeap()->SetName(L"RTV Heap"), , "Could not name RTV Heap.");

	heap = GetDSVHeap();
	result |= heap->Setup(512, false);
	HRESULT_CHECKED_FALLBACK(heap->GetHeap()->SetName(L"DSV Heap"), , "Could not name DSV Heap.");

	heap = GetDSVHeap();
	result |= heap->Setup(4096, true);
	HRESULT_CHECKED_FALLBACK(heap->GetHeap()->SetName(L"SRV Heap"), , "Could not name SRV Heap.");

	heap = GetDSVHeap();
	result |= heap->Setup(512, false);
	HRESULT_CHECKED_FALLBACK(heap->GetHeap()->SetName(L"UAV Heap"), , "Could not name UAV Heap.");
}

void DX12::Render()
{
	DX12Command* command = GetDx12MainRenderCommand();
	command->PreRender();

	ID3D12GraphicsCommandList6* clist = command->GetCommandList();
	
	::PendingResourceFreeingRoutine();


	command->PostRender();
}

void DX12::Cleanup()
{
	// intended to be called only once at shutdown, reinit is not possible due to static nature of things?
	auto cleaner = []()
		{
			GetDx12MainRenderCommand()->Release();
			GetRTVHeap()->Release();
			GetDSVHeap()->Release();
			GetSRVHeap()->Release();
			GetUAVHeap()->Release();
			::COMDxgiFactory.Reset();
			::COMRendererDevice.Reset();
			::COMDxgiAdapter.Reset();
		};
			
#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather info queue interface.");
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);

		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugInterface = nullptr;
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12DebugDevice2), (void**)debugInterface.GetAddressOf()), "Couldn't gather debug device interface.");

		cleaner();

		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#else
	cleaner();
#endif
}

DX12::DX12Command::DX12Command(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC cq;
	cq.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cq.NodeMask = 0;
	cq.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cq.Type = type;

	const wchar_t* commandQueueName = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Queue" :
		type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"GFX Compute Queue" :
		L"Command Queue";

	const wchar_t* commandListName = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command List" :
		type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"GFX Compute List" :
		L"Command List";

	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateCommandQueue(&cq, __uuidof(ID3D12CommandQueue), (void**)_commandQueue.GetAddressOf()),
		goto _error,
		"Could not create command queue.");

	for (int i = 0; i < k_frameBuffers; i++)
		_frame[i].Setup(type, i);

	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateCommandList(0, type, _frame[0].GetAllocator(), nullptr, __uuidof(ID3D12GraphicsCommandList6), (void**)_commandList.GetAddressOf()),
		goto _error,
		"Could not create command list.");

	HRESULT_CHECKED_FALLBACK(_commandQueue->SetName(commandQueueName), , "Could not set (%ls) to command queue.", commandQueueName);
	HRESULT_CHECKED_FALLBACK(_commandList->SetName(commandListName), , "Could not set (%ls) to command list.", commandListName);

	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence1), (void**)_fence.GetAddressOf()),
		goto _error,
		"Could not create fence.");

	HRESULT_CHECKED_FALLBACK(_commandList->SetName(L"D3D12 Fence"), , "Could not set (%ls) to command list.", commandListName);

	_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	if(!_fenceEvent)
		DISPLAY_ERROR(L"Could not create fence event.");

	return;

_error:

	Release();
}

void DX12::DX12Command::PreRender()
{
	DX12CommandFrame& currentFrame = _frame[_currentFrame];
	currentFrame.WaitForGPU(_fenceEvent, _fence.Get());
	HRESULT_CHECKED(currentFrame.GetAllocator()->Reset(), "Could not clear/reset commandAllocator");
	HRESULT_CHECKED(_commandList->Reset(currentFrame.GetAllocator(), nullptr), "Could not reset command list.");
}

void DX12::DX12Command::PostRender()
{
	_commandList->Close();
	ID3D12CommandList* const lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(lists), lists);

	_issuableFenceValue++;
	_frame[_currentFrame].IssueFenceValue(_issuableFenceValue);
	_commandQueue->Signal(_fence.Get(), _issuableFenceValue);

	_currentFrame = ++_currentFrame % k_frameBuffers;
}

void DX12::DX12Command::Flush()
{
	for (int i = 0; i < k_frameBuffers; i++)
		_frame[i].WaitForGPU(_fenceEvent, _fence.Get());

	_issuableFenceValue = 0;
}

void DX12::DX12Command::Release()
{
	Flush();

	_fence.Reset();
	if (_fenceEvent)
	{
		CloseHandle(_fenceEvent);
		_fenceEvent = 0;
	}

	_commandList.Reset();
	_commandQueue.Reset();
	for (int i = 0; i < k_frameBuffers; i++)
		_frame[i].Reset();

	_currentFrame = 0;
}

bool DX12::DX12Command::DX12CommandFrame::Setup(D3D12_COMMAND_LIST_TYPE type, int index)
{
	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateCommandAllocator(type, _uuidof(ID3D12CommandAllocator), (void**)_allocator.GetAddressOf()),
		return false,
		"Could not create command allocator.");

	wchar_t buffer[250];
	swprintf(buffer, 250, L"Command Allocator %u", index);
	HRESULT_CHECKED_FALLBACK(_allocator->SetName(buffer), , "Could not set name (%ls) to command buffer.", buffer);

	return true;
}

void DX12::DX12Command::DX12CommandFrame::Reset()
{
	_allocator.Reset();
	_fenceValue = 0;
}

void DX12::DX12Command::DX12CommandFrame::WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence)
{
	assert(fenceEvent && fence);

	if (fence->GetCompletedValue() >= _fenceValue)
		return;
	
	HRESULT_CHECKED(fence->SetEventOnCompletion(_fenceValue, fenceEvent), "Could not assign fence signal to event handle.");

	WaitForSingleObject(fenceEvent, INFINITE);
}
