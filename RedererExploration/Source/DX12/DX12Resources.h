#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"

namespace DX12
{
	namespace PipelineStream
	{
		template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type, typename T>
		class alignas(void*) PipelineSubobjectType
		{
		public:

			PipelineSubobjectType() = default;
			PipelineSubobjectType(T subobject) : _type(type), _subobject(subobject) {}
			PipelineSubobjectType& operator=(const T& subobject) { _subobject = subobject; return *this; }

		private:
			const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _type{ type };
			T _subobject;
		};

		using SubobjectRootSignature = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
		using SubobjectVertexShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;
		using SubobjectPixelShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS, D3D12_SHADER_BYTECODE>;
		using SubobjectDomainShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS, D3D12_SHADER_BYTECODE>;
		using SubobjectHullShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS, D3D12_SHADER_BYTECODE>;
		using SubobjectGeometryShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS, D3D12_SHADER_BYTECODE>;
		using SubobjectComputeShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS, D3D12_SHADER_BYTECODE>;
		using SubobjectAmplificationShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS, D3D12_SHADER_BYTECODE>;
		using SubobjectMeshShader = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS, D3D12_SHADER_BYTECODE>;
		using SubobjectStreamOutput = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT, D3D12_STREAM_OUTPUT_DESC>;
		using SubobjectBlend = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, D3D12_BLEND_DESC>;
		using SubobjectSampleMask = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, unsigned int>;
		using SubobjectRasterizer = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, D3D12_RASTERIZER_DESC>;
		using SubobjectDepthStencil = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, D3D12_DEPTH_STENCIL_DESC>;
		using SubobjectInputLayout = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT, D3D12_INPUT_LAYOUT_DESC>;
		using SubobjectIndexBufferStripCutValue = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE>;
		using SubobjectPrimitiveTopology = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, D3D12_PRIMITIVE_TOPOLOGY_TYPE>;
		using SubobjectRenderTargetFormats = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY>;
		using SubobjectDepthStencilFormats = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT, DXGI_FORMAT>;
		using SubobjectSampleDesc = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DXGI_SAMPLE_DESC>;
		using SubobjectNodeMask = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK, unsigned int>;
		using SubobjectCachedPipelineStateObject = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO, D3D12_CACHED_PIPELINE_STATE>;
		using SubobjectDepthStencil1 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, D3D12_DEPTH_STENCIL_DESC1>;
		using SubobjectViewInstancing = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, D3D12_VIEW_INSTANCING_DESC>;
		using SubobjectepthStencil2 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2, D3D12_DEPTH_STENCIL_DESC2>;
		using SubobjectRasterizer1 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER1, D3D12_RASTERIZER_DESC1>;
		using SubobjectRasterizer2 = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER2, D3D12_RASTERIZER_DESC2>;
		using SubobjectSerializedRootSignature = PipelineSubobjectType<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SERIALIZED_ROOT_SIGNATURE, D3D12_SERIALIZED_ROOT_SIGNATURE_DESC>;		
	} // PipelineStream;

	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE _cpu{ 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE _gpu{ 0 };

		DescriptorHandle() = default;
		DescriptorHandle(const DescriptorHandle&) = default;
		DescriptorHandle& operator=(const DescriptorHandle& other) = default;
		DescriptorHandle(DescriptorHandle&& other) noexcept { MoveImpl(std::move(other)); };
		DescriptorHandle& operator=(DescriptorHandle&& other) noexcept { return MoveImpl(std::move(other)); };

		constexpr bool IsValid() const { return _cpu.ptr != 0; }
		constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }

	private:

		DescriptorHandle& MoveImpl(DescriptorHandle&& other);

#ifdef _DEBUG
	private:

		friend class DescriptorHeap;

		DescriptorHeap* _owner = nullptr;
		unsigned int _index = 0;
#endif
	};

	struct RenderTargetData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
		DX12::DescriptorHandle _rtvHandle = {};

		RenderTargetData() = default;
		RenderTargetData(RenderTargetData&& other) noexcept { MoveImpl(std::move(other)); }
		RenderTargetData& operator=(RenderTargetData&& other) noexcept { return MoveImpl(std::move(other)); }

		NO_COPY(RenderTargetData)

		void Release();

	private:

		RenderTargetData& MoveImpl(RenderTargetData&& other);
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
		{};
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

	struct RootSignatureSpec : public D3D12_ROOT_SIGNATURE_DESC2 
	{
		RootSignatureSpec(
			const DX12::RootParameter* parameters,
			unsigned int parameterCount,
			const D3D12_STATIC_SAMPLER_DESC1* samplers = nullptr,
			unsigned int samplerCount = 0,
			D3D12_ROOT_SIGNATURE_FLAGS flags = DX12::CC::RootSignatureSpec::k_PixelStageOnly)
		: D3D12_ROOT_SIGNATURE_DESC2{parameterCount, parameters, samplerCount, samplers, flags}
		{};
	};
}