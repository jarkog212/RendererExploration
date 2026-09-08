#include "DX12Camera.h"
#include "Utils.h"

void DX12::Projection::Camera::SetUp(DirectX::XMFLOAT3& up)
{
	_specs._up = up;
	_dirty = true;
}

void DX12::Projection::Camera::SetNearZ(float nearZ)
{
	_specs._nearZ = nearZ;
	_dirty = true;
}

void DX12::Projection::Camera::SetFarZ(float farZ)
{
	_specs._farZ = farZ;
	_dirty = true;
}

void DX12::Projection::Camera::SetFov(float fov)
{
	_specs._fov = fov;
	_dirty = true;
}

void DX12::Projection::Camera::SetAspectRatio(float aspectRatio)
{
	_specs._aspectRatio = aspectRatio;
	_dirty = true;
}

void DX12::Projection::Camera::SetViewWidth(float viewWidth)
{
	FALLBACK_ASSERT(_specs._type == DX12::Projection::Type::Orthogonal,,, "Setting view width for non orthogonal camera.");
	_specs._viewWidth = viewWidth;
	_dirty = true;
}

void DX12::Projection::Camera::SetViewHeight(float viewHeight)
{
	FALLBACK_ASSERT(_specs._type == DX12::Projection::Type::Orthogonal,,, "Setting view height for non orthogonal camera.");
	_specs._viewHeight = viewHeight;
	_dirty = true;
}

void DX12::Projection::Camera::Update(float deltaTime)
{
	auto* translation = GetAttribute<ATransform>();

}
