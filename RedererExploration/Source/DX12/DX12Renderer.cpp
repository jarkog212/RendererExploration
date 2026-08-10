#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"
#include "DX12RenderSurface.h"
#include "DX12Shaders.h"

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12Device8> COMRendererDevice = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIFactory7> COMDxgiFactory = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIAdapter4> COMDxgiAdapter = nullptr;

	static std::unique_ptr<DX12::RenderSurface> g_mainRenderSurface = nullptr;
	static D3D_FEATURE_LEVEL g_expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };
	static int g_deferredResourceFreeingFlag = 0;
	static_assert(sizeof(g_deferredResourceFreeingFlag) * sizeof(char) > DX12::CC::k_frameBuffers, "Flag bit-buffer needs to be wide enough for frame buffer count.");
	static std::vector<IUnknown*> g_deferredCOMReleases[DX12::CC::k_frameBuffers];

	static void FreePendingResources(const int currentFrame = DX12::CC::k_forAllFrames)
	{
		static std::mutex mutex;
		std::lock_guard lock(mutex);

		::g_deferredResourceFreeingFlag &= ~(1ull << currentFrame);

		DX12::GetDSVHeap()->ProcessPendingResourceFreeing(currentFrame);
		DX12::GetRTVHeap()->ProcessPendingResourceFreeing(currentFrame);
		DX12::GetSRVHeap()->ProcessPendingResourceFreeing(currentFrame);
		DX12::GetUAVHeap()->ProcessPendingResourceFreeing(currentFrame);

		static auto cleaner = [](const unsigned int frame)
			{
				std::vector<IUnknown*>& pendingCOMList = g_deferredCOMReleases[frame];
				for (auto& comObj : pendingCOMList)
					Utils::COM::SafeRelease(comObj);

				pendingCOMList.clear();
			};

		if (currentFrame == DX12::CC::k_forAllFrames)
			for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
				cleaner(i);
		else
			cleaner(currentFrame);
	}

	namespace Example 
	{
		static Microsoft::WRL::ComPtr<ID3D12RootSignature> COMRenderRootSignature = nullptr;
		static Microsoft::WRL::ComPtr<ID3D12PipelineState> COMRenderPipelineState = nullptr;
	}
}

void UtilsImpl::COM::Release(IUnknown* resource)
{
	resource->Release();
}

void UtilsImpl::COM::DeferredRenderThreadRelease(IUnknown* resource)
{
	unsigned int currentFrame = DX12::GetMainCommandExecutor()->GetCurrentFrame();
	g_deferredCOMReleases[currentFrame].push_back(resource);
}

ID3D12Device8* DX12::GetRendererDevice()
{
	ID3D12Device8* device = ::COMRendererDevice.Get();
	if (device)
		return device;

	HRESULT_CHECK_CRITICAL(
		D3D12CreateDevice(DX12::GetDxgiAdapter(), g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device8), (void**)&device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECK_FALLBACK(device->SetName(L"MainDevice"),,, "Could not set Dx12 device's name.");

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECK_CRITICAL(device->QueryInterface(IID_PPV_ARGS(&infoQueueInterface)), "Couldn't gather queue interface.");
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
		HRESULT_CHECK_CRITICAL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)), "Failed to get debug interface.");
		debugInterface->EnableDebugLayer();

		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT_CHECK_CRITICAL(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)), "DXGI Factory issue.");

	::COMDxgiFactory.Attach(factory);
	return factory;
}

IDXGIAdapter4* DX12::GetDxgiAdapter()
{
	IDXGIAdapter4* adapter = ::COMDxgiAdapter.Get();
	if (adapter)
		return adapter;

	for (unsigned int i = 0;
		DX12::GetDxgiFactory()->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter4), (void**)&adapter) != DXGI_ERROR_NOT_FOUND;
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

	FALLBACK_ASSERT(adapter, return nullptr, "No good GPU adapter.");

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
	HRESULT_CHECK_CRITICAL(
		D3D12CreateDevice(adapter, ::g_expectedMinimumFeatureLevel, IID_PPV_ARGS(&device)),
		"Could not create Dx12 device with selected adapter.");
	
	HRESULT_CHECK_CRITICAL(
		device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData)),
		"Could not access Dx12 device's feature support information.");

	FALLBACK_ASSERT(
		featureLevelsData.MaxSupportedFeatureLevel >= ::g_expectedMinimumFeatureLevel,
		{
			Utils::COM::SafeRelease(adapter);
			return nullptr;
		},
		"Dx12 device's feature level support is lower than minimum requirement.");

	::g_expectedMinimumFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
	::COMDxgiAdapter.Attach(adapter);
	return adapter;
}

DX12::RenderSurface* DX12::GetMainRenderSurface(
	HWND window,
	DXGI_FORMAT format,
	ID3D12CommandQueue* commandQueue)
{
	if (::g_mainRenderSurface)
		return ::g_mainRenderSurface.get();

	CRITICAL_ASSERT(
		window != NULL && commandQueue,
		"Uninitialized render surface requires legitimate arguments so it can be initialized.");

	::g_mainRenderSurface = std::make_unique<DX12::RenderSurface>(window, format, commandQueue);
	return ::g_mainRenderSurface.get();
}

DX12::GfxCommandExecutor* DX12::GetMainCommandExecutor()
{
	static DX12::GfxCommandExecutor instance(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return &instance;
}

DX12::DescriptorHeap* DX12::GetRTVHeap()
{
	static DX12::DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 512, false);
	return &instance;
}

DX12::DescriptorHeap* DX12::GetDSVHeap()
{
	static DX12::DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 512, false);
	return &instance;
}

DX12::DescriptorHeap* DX12::GetSRVHeap()
{
	static DX12::DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, true);
	return &instance;
}

DX12::DescriptorHeap* DX12::GetUAVHeap()
{
	static DX12::DescriptorHeap instance(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512, false);
	return &instance;
}

void DX12::SetPendingResourceFreeingFlag()
{
	::g_deferredResourceFreeingFlag |= 1ull << DX12::GetMainCommandExecutor()->GetCurrentFrame();
}

bool DX12::TestPendingResourceFreeingFlag(const unsigned int frame)
{
	return (bool)(::g_deferredResourceFreeingFlag & (1ull << frame));
}

bool DX12::Initialize(HWND hwnd)
{
	bool result = true;

	DX12::GetMainRenderSurface(
		hwnd,
		DX12::CC::k_defaultRenderTargetFormat,
		DX12::GetMainCommandExecutor()->GetCommandQueue());

	HRESULT_CHECK_FALLBACK(DX12::GetRTVHeap()->GetHeap()->SetName(L"RTV Heap"),,, "Could not name RTV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"DSV Heap"),,, "Could not name DSV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"SRV Heap"),,, "Could not name SRV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"UAV Heap"),,, "Could not name UAV Heap.");

	return true;
}

void DX12::Render(DX12::GfxCommandExecutor* command, DX12::RenderSurface* surface)
{
	command->PreRender();

	ID3D12GraphicsCommandList6* clist = command->GetCommandList();
	
	const unsigned int currentFrame = DX12::GetMainCommandExecutor()->GetCurrentFrame();
	if (TestPendingResourceFreeingFlag(currentFrame))
		FreePendingResources(currentFrame);

	surface->Present();

	command->PostRender();
}

void DX12::Render()
{
	DX12::Render(
		DX12::GetMainCommandExecutor(),
		DX12::GetMainRenderSurface());
}

void DX12::Release()
{
	// intended to be called only once at shutdown, reinit is not possible due to static nature of things?
	static auto cleaner = []()
		{
			DX12::Shaders::Release();

			::g_mainRenderSurface.reset();

			DX12::GetMainCommandExecutor()->Release();

			::FreePendingResources(DX12::CC::k_forAllFrames);

			DX12::GetRTVHeap()->Release();
			DX12::GetDSVHeap()->Release();
			DX12::GetSRVHeap()->Release();
			DX12::GetUAVHeap()->Release();

			// Heap releases add to the deferred pile
			::FreePendingResources(DX12::CC::k_forAllFrames);

			::Example::COMRenderRootSignature.Reset();
			::Example::COMRenderPipelineState.Reset();

			::COMDxgiFactory.Reset();
			::COMRendererDevice.Reset();
			::COMDxgiAdapter.Reset();
		};
			
#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECK_CRITICAL(::COMRendererDevice->QueryInterface(IID_PPV_ARGS(&infoQueueInterface)), "Couldn't gather info queue interface.");
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);

		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugInterface = nullptr;
		HRESULT_CHECK_CRITICAL(::COMRendererDevice->QueryInterface(IID_PPV_ARGS(&debugInterface)), "Couldn't gather debug device interface.");

		cleaner();

		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#else
	cleaner();
#endif
}

DX12::GfxCommandExecutor::GfxCommandExecutor(D3D12_COMMAND_LIST_TYPE type)
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

	ID3D12Device8* device = DX12::GetRendererDevice();

	HRESULT_CHECK_FALLBACK(
		device->CreateCommandQueue(&cq, IID_PPV_ARGS(&_commandQueue)),,
		goto _error,
		"Could not create command queue.");

	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].Setup(type, i);

	HRESULT_CHECK_FALLBACK(
		device->CreateCommandList(0, type, _frame[0].GetAllocator(), nullptr, IID_PPV_ARGS(&_commandList)),,
		goto _error,
		"Could not create command list.");

	HRESULT_CHECK_FALLBACK(
		_commandList->Close(),,
		goto _error,
		"Could not close the command list.")

	HRESULT_CHECK_FALLBACK(_commandQueue->SetName(commandQueueName),,, "Could not set (%ls) to command queue.", commandQueueName);
	HRESULT_CHECK_FALLBACK(_commandList->SetName(commandListName),,, "Could not set (%ls) to command list.", commandListName);

	HRESULT_CHECK_FALLBACK(
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),,
		goto _error,
		"Could not create fence.");

	HRESULT_CHECK_FALLBACK(_commandList->SetName(L"D3D12 Fence"),,, "Could not set (%ls) to command list.", commandListName);

	_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	FALLBACK_ASSERT(_fenceEvent,,"Could not create fence event.");

	return;

_error:

	Release();
}

void DX12::GfxCommandExecutor::PreRender()
{
	DX12::GfxCommandExecutor::GfxCommandFrame& currentFrame = _frame[_currentFrame];
	currentFrame.WaitForGPU(_fenceEvent, _fence.Get());
	HRESULT_CHECK_CRITICAL(currentFrame.GetAllocator()->Reset(), "Could not clear/reset commandAllocator");
	HRESULT_CHECK_CRITICAL(_commandList->Reset(currentFrame.GetAllocator(), nullptr), "Could not reset command list.");
}

void DX12::GfxCommandExecutor::PostRender()
{
	_commandList->Close();
	ID3D12CommandList* const lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(lists), lists);

	_issuableFenceValue++;
	_frame[_currentFrame].IssueFenceValue(_issuableFenceValue);
	_commandQueue->Signal(_fence.Get(), _issuableFenceValue);

	_currentFrame = ++_currentFrame % DX12::CC::k_frameBuffers;
}

void DX12::GfxCommandExecutor::Flush()
{
	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].WaitForGPU(_fenceEvent, _fence.Get());

	_issuableFenceValue = 0;
}

void DX12::GfxCommandExecutor::Release()
{
	if (_fence)
	{
		Flush();
		_fence.Reset();
		if (_fenceEvent)
		{
			CloseHandle(_fenceEvent);
			_fenceEvent = 0;
		}
	}

	_commandList.Reset();
	_commandQueue.Reset();
	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].Reset();

	_currentFrame = 0;
}

bool DX12::GfxCommandExecutor::GfxCommandFrame::Setup(D3D12_COMMAND_LIST_TYPE type, int index)
{
	HRESULT_CHECK_FALLBACK(
		DX12::GetRendererDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&_allocator)),,
		return false,
		"Could not create command allocator.");

	wchar_t buffer[250];
	swprintf(buffer, 250, L"Command Allocator %u", index);
	HRESULT_CHECK_FALLBACK(_allocator->SetName(buffer),,,"Could not set name (%ls) to command buffer.", buffer);

	return true;
}

void DX12::GfxCommandExecutor::GfxCommandFrame::Reset()
{
	_allocator.Reset();
	_fenceValue = 0;
}

void DX12::GfxCommandExecutor::GfxCommandFrame::WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence)
{
	assert(fenceEvent && fence);

	if (fence->GetCompletedValue() >= _fenceValue)
		return;
	
	HRESULT_CHECK_CRITICAL(fence->SetEventOnCompletion(_fenceValue, fenceEvent), "Could not assign fence signal to event handle.");

	WaitForSingleObject(fenceEvent, INFINITE);
}

ID3D12GraphicsCommandList6* DX12::Example::ExampleRenderBuild()
{
	DX12::DescriptorHeap* srvHeap = DX12::GetSRVHeap();

	ID3D12GraphicsCommandList6* commandList = DX12::GetMainCommandExecutor()->GetCommandList();

	commandList->SetGraphicsRootSignature(DX12::Example::GetRootSignature());
	commandList->SetPipelineState(DX12::Example::GetPipelineState());

	ID3D12DescriptorHeap* heaps[]{ srvHeap->GetHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	commandList->SetGraphicsRootDescriptorTable(0, srvHeap->GetGpuPtr());

	commandList->SetGraphicsRoot32BitConstant(1, (int)Utils::GetFPS(), 0);
	commandList->SetGraphicsRoot32BitConstant(1, 42, 1);

	//commandList->SetGraphicsRootConstantBufferView(1,);

	return commandList;
}

ID3D12RootSignature* DX12::Example::GetRootSignature()
{
	if (::Example::COMRenderRootSignature)
		return ::Example::COMRenderRootSignature.Get();

	// Create signature = memory layout for buffers
	DX12::RootParameter rootParams[2]{};
	DX12::DescriptorRange descriptorRanges[]{
		DX12::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, DX12::GetSRVHeap()->GetCapacity(), 0) };

	rootParams[0].AsDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, descriptorRanges, 1);

	rootParams[1].AsConstants(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);

	DX12::RootSignatureSpec rootSpec(rootParams, _countof(rootParams));
	::Example::COMRenderRootSignature = DX12::CreateRootSignature(rootSpec);

	return ::Example::COMRenderRootSignature.Get();
}

ID3D12PipelineState* DX12::Example::GetPipelineState()
{
	if (::Example::COMRenderPipelineState)
		return ::Example::COMRenderPipelineState.Get();

	struct {
		DX12::PipelineStream::SubobjectRootSignature rs{ DX12::Example::GetRootSignature() };
	}stream;

	::Example::COMRenderPipelineState = DX12::CreatePipelineState(&stream, sizeof(stream));
	
	return ::Example::COMRenderPipelineState.Get();
}
