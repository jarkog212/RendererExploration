#pragma once
#include "DX12Utils.h"
#include "DX12Shaders.h"
#include "DX12DescriptorHeap.h"

#include "Utils.h"

#include "DirectXMath.h"

#include <vector>

#define MATERIAL_LIST_OP(XX) \
    XX(DX12::Materials::Type::Basic, DX12::MaterialLib::BasicMaterial) \

namespace DX12::Materials
{
    enum class Type
    {
        Basic,

        Count,
        Invalid
    };

    class Material
    {
    public:

        Material(DX12::Materials::Type type) : _type(type) {};
        virtual ~Material() {};
        Material(Material&&) = default;
        Material& operator=(Material&&) = default;

        NO_COPY(Material);

        virtual ID3D12PipelineState* GetPSO() const = 0;
        virtual unsigned int GetConstantBufferSRVIndex() const = 0;
        virtual void AccummulateMaterialCommands(int currentFrame, ID3D12GraphicsCommandList6* commandList) {};

        DX12::Materials::Type GetType() { return _type; }

    protected:

        const DX12::Materials::Type _type = DX12::Materials::Type::Invalid;
    };
}

namespace DX12::MaterialSpace
{
    void Release();
    void WarmUp();

    std::shared_ptr<DX12::Materials::Material> GetOrLoad(DX12::Materials::Type type);
}

namespace DX12::MaterialLib
{
    // - Basic Material -
    class BasicMaterial : public DX12::Materials::Material
    {
    public:

        BasicMaterial();
        ~BasicMaterial();
        BasicMaterial(BasicMaterial&&) = default;
        BasicMaterial& operator=(BasicMaterial&&) = default;

        ID3D12PipelineState* GetPSO() const final override;
        unsigned int GetConstantBufferSRVIndex() const final override;
        void AccummulateMaterialCommands(int currentFrame, ID3D12GraphicsCommandList6* commandList) final override;
  
        static const DX12::Shaders::Specs& GetPixelShaderSpec();
        static const DX12::Shaders::Specs& GetVertexShaderSpec();

    protected:

        std::shared_ptr<DX12::Shaders::Shader> _pixelShader = nullptr;
        std::shared_ptr<DX12::Shaders::Shader> _vertexShader = nullptr;
        ID3D12Resource* _constStructuredBuffer = nullptr; // non owning deferred free 
        ID3D12PipelineState* _pipelineStateObject = nullptr; // non owning deferred free 
        DX12::DescriptorHandle _structuredConstantBufferSRVHandle = {};

        struct BasicStructuredBuffer
        {
            DirectX::XMFLOAT3 _baseMeshColor = { 1.f, 0.f, 1.f };
        }_mappedConstantStructuredBuffer;
    };
}