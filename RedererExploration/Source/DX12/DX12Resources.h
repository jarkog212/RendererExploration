#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"

namespace DX12
{
	namespace PipelineStream
	{
		template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, typename T>
		struct alignas(void*) PipelineSubobjectType
		{
			PipelineSubobjectType() = default;
			PipelineSubobjectType(T subobject) : _type(type), _sub(subobject) {}
			PipelineSubobjectType& operator=(const T& subobject) { _sub = subobject; return *this; }

			const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _type{ type };
			T _sub;
		};

		using SubRootSignature = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
		using SubVertexShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;
		using SubPixelShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
		using SubDomainShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE>;
		using SubHullShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE>;
		using SubGeometryShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE>;
		using SubComputeShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE>;
		using SubAmplificationShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>;
		using SubMeshShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>;
		using SubStreamOutput = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC>;
		using SubBlend = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
		using SubSampleMask = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, unsigned int>;
		using SubRasterizer = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
		using SubDepthStencil = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
		using SubInputLayout = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>;
		using SubIndexBufferStripCutValue = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>;
		using SubPrimitiveTopology = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>;
		using SubRenderTargetFormats = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
		using SubDepthStencilFormat = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
		using SubSampleDesc = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;
		using SubNodeMask = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, unsigned int>;
		using SubCachedPipelineStateObject = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE>;
		using SubDepthStencil1 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1>;
		using SubViewInstancing = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC>;
		using SubDepthStencil2 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2>;
		using SubRasterizer1 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1, D3D12_RASTERIZER_DESC1>;
		using SubRasterizer2 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2>;
		using SubSerializedRootSignature = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SERIALIZED_ROOT_SIGNATURE, D3D12_SERIALIZED_ROOT_SIGNATURE_DESC>;
	} // PipelineStream;

	struct DescriptorHandleView
	{
		D3D12_CPU_DESCRIPTOR_HANDLE _cpu{ 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE _gpu{ 0 };

		constexpr bool IsValid() const { return _cpu.ptr != 0; }
		constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }
	};

	struct DescriptorHandle : public DescriptorHandleView
	{
		DescriptorHandle() = default;
		~DescriptorHandle();
		DescriptorHandle(DescriptorHandle&& other) noexcept;
		DescriptorHandle& operator=(DescriptorHandle&& other) noexcept;

		NO_COPY(DescriptorHandle);

		constexpr bool IsValid() const { return _cpu.ptr != 0; }
		constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }
		const DX12::DescriptorHandleView CreateView() const { return { _cpu , _gpu }; };

		DX12::DescriptorHeap* _owner = nullptr;

#ifdef _DEBUG
	private:

		friend class DX12::DescriptorHeap;

		unsigned int _index = 0;
#endif
	};

	struct RenderTargetData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
		DX12::DescriptorHandle _rtvHandle = {};

		RenderTargetData() = default;
		RenderTargetData(RenderTargetData&& other) noexcept;
		RenderTargetData& operator=(RenderTargetData&& other) noexcept;

		NO_COPY(RenderTargetData);

		void Release() { if (_rtvHandle._owner) *this = RenderTargetData(); }
	};

	struct DescriptorRange : public D3D12_DESCRIPTOR_RANGE1
	{
		DescriptorRange(
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
			unsigned int descriptorCount,
			unsigned int shaderRegister,
			unsigned int space = 0,
			D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE,
			unsigned int offsetFromTable = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
			: D3D12_DESCRIPTOR_RANGE1{ rangeType, descriptorCount, shaderRegister, space, flags, offsetFromTable }
		{
		};
	};

	struct RootParameter : public D3D12_ROOT_PARAMETER1
	{
		void AsConstants(
			unsigned int numConstants,
			D3D12_SHADER_VISIBILITY visibility,
			unsigned int shaderRegister,
			unsigned int space = 0);

		void AsCBV(
			D3D12_SHADER_VISIBILITY visibility,
			unsigned int shaderRegister,
			unsigned int space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE);

		void AsSRV(
			D3D12_SHADER_VISIBILITY visibility,
			unsigned int shaderRegister,
			unsigned int space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE);

		void AsUAV(
			D3D12_SHADER_VISIBILITY visibility,
			unsigned int shaderRegister,
			unsigned int space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE);

		void AsDescriptorTable(D3D12_SHADER_VISIBILITY visibility, DX12::DescriptorRange* ranges, unsigned int count);

	private:

		constexpr void AsDescriptor(
			unsigned int shaderRegister,
			unsigned int space = 0,
			D3D12_ROOT_DESCRIPTOR_FLAGS flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
	};

	class RenderPassBase
	{
	public:

		void SetNext(RenderPassBase& pass) { _next = &pass; };
		void UnsetNext() { _next = nullptr; };
		void AccummulateCommands(ID3D12GraphicsCommandList6* commandList);
		void SetAutoBarrierSubmits(bool prePass, bool postPass) { _submitBarriers[0] = prePass; _submitBarriers[1] = postPass; };

	protected:

		virtual void PrePassResourceBarriers() = 0;
		virtual void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) = 0;
		virtual void PostPassResourceBarriers() = 0;

		const char* _name = "";

	private:

		RenderPassBase* _next = nullptr;

		// true, false as we expect the post pass resource
		//   barriers and pre pass resource barriers can be grouped usually.
		bool _submitBarriers[2] = { true, false };
	};


}