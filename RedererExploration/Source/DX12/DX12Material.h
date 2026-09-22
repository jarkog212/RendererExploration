#pragma once
#include "DX12Utils.h"

#include "Utils.h"

#include <vector>

#define MATERIAL_LIST_OP(XX) \
    XX(DX12::Materials::Type::Basic, DX12::Materials::BasicMaterial) \

namespace DX12::Materials
{
    enum class Type
    {
        Basic,

        Count
    };

    class Material
    {
    public:

        Material() = default;
        virtual ~Material() = default;

        NO_COPY(Material);

        virtual ID3D12PipelineState* GetPSO() const = 0;
        virtual int GetDataIndecesBufferIndex() const = 0;
        virtual void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) = 0;
        virtual void Load() = 0;
        virtual void Release() = 0;
    };

    const Material& GetOrLoad(DX12::Materials::Type type);
    void Unload(DX12::Materials::Type type);
    void Release();
    void WarmUp();

    class BasicMaterial : public Material
    {
    public:

        BasicMaterial() {};
        ~BasicMaterial() { DEBUG_DESTRUCTOR_BODY(BasicMaterial); Release(); }
        BasicMaterial(BasicMaterial&&) = default;
        //BasicMaterial& operator=(BasicMaterial&&);

        ID3D12PipelineState* GetPSO() const final override;
        int GetDataIndecesBufferIndex() const final override;
        void AccummulatePassCommands(ID3D12GraphicsCommandList6* commandList) final override;
        void Load() final override;
        void Release() final override;

    protected:


    };

}