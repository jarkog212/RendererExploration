#include "DX12UploadContext.h"
#include "DX12Utils.h"

DX12::GPUUploadContext::GPUUploadContext(unsigned int alignedSize, const void* data)
{
	CRITICAL_ASSERT(data, "Cannot make upload context without valid data to upload.");

	memcpy(_address, data, alignedSize);
}