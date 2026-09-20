#pragma once
#include "DX12Utils.h"

#include "Utils.h"

#define MATERIAL_LIST_OP(XX) \
    //XX(DX12::Materials::Type::Base, DX12::Materials::Material) \

namespace DX12::Materials
{
    enum class Type
    {
        Base,

        Count
    };

    class Material
    {
    public:

        Material() = default;
        virtual ~Material() = 0;
        
        NO_COPY(Material);

        virtual ID3D12PipelineState* GetPSO() const = 0;
        virtual void Load() = 0;
        virtual void Release() = 0;
    };

    const Material& Get(DX12::Materials::Type type);
    void Unload(DX12::Materials::Type type);
    void Release();
    void WarmUp();
}