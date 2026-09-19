#pragma once
#include "DX12Utils.h"

namespace DX12::Materials
{
    class Material
    {

    };

    int Add(const void* stream, unsigned long long& consumedBytes);
    void Remove(unsigned int index);
    void Release();
    void WarmUp();
}