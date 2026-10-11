#include "DX12Attributes.h"

void DX12::Attributes::Transform::PrecalculateWorld()
{
	// Z axis = forward (fall back to +Z if zero-length)
	DirectX::XMVECTOR z = DirectX::XMVectorSetW(_transform._forward, 0.0f);
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(z)) < 1e-12f)
		z = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	z = DirectX::XMVector3Normalize(z);

	// X axis = right = up x forward (left-handed)
	DirectX::XMVECTOR up = DirectX::XMVectorSetW(_transform._up, 0.0f);
	DirectX::XMVECTOR x = DirectX::XMVector3Cross(up, z);

	// Degenerate: up is zero-length or parallel to forward, so pick a fallback up
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(x)) < 1e-12f)
	{
		up = (std::fabs(DirectX::XMVectorGetY(z)) < 0.999f)
			? DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
			: DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		x = DirectX::XMVector3Cross(up, z);
	}
	x = DirectX::XMVector3Normalize(x);

	// Y axis = true up, re-orthogonalized (already unit length)
	DirectX::XMVECTOR y = DirectX::XMVector3Cross(z, x);

	// In row-vector convention the rows of the rotation matrix are the basis axes
	DirectX::XMMATRIX rotation(x, y, z, DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

	DirectX::XMMATRIX scale = DirectX::XMMatrixScalingFromVector(a._scale);
	DirectX::XMMATRIX translate = DirectX::XMMatrixTranslationFromVector(a._location);

	return scale * rotation * translate;
}

void DX12::Attributes::GPUMappedTransform::LinkToGPUArena(void* Key)
{
}

void DX12::Attributes::GPUMappedTransform::PrecalculateWorld()
{
	DX12::Attributes::Transform::PrecalculateWorld();


}

void DX12::Attributes::Movable::Reset()
{
	_relativeTranslate = {};
	_worldTranslate = {};
	_rotate = {};
}

void DX12::Attributes::Movable::Apply()
{
	DX12::Attributes::Transform::AttributeType transform = *_transformToModify;

	// world translate
	if (_worldTranslate.m128_f32[0] || _worldTranslate.m128_f32[1] || _worldTranslate.m128_f32[2])
		transform._location = DirectX::XMVectorAdd(transform._location, _worldTranslate);

	// rotation
	if (_rotate.x != 0.f || _rotate.y != 0.f || _rotate.z != 0.f)
	{
		// Roll = about Forward = Z Axis, Pitch = about Right = X Axis, Yaw = about Up = Y Axis
		DirectX::XMVECTOR quat = DirectX::XMQuaternionRotationRollPitchYaw(_rotate.x, _rotate.y, _rotate.z);
		transform._forward = DirectX::XMVector3Rotate(transform._forward, quat);
		transform._up = DirectX::XMVector3Rotate(transform._up, quat);
	}

	if (const float right = _relativeTranslate.m128_f32[0]; right != 0.f)
	{
		const DirectX::XMVECTOR rightAxis = DirectX::XMVector3Cross(transform._up, transform._forward);
		transform._location = DirectX::XMVectorMultiplyAdd(
			rightAxis,
			{ right, right, right, 0 },
			transform._location);
	}

	if (const float up = _relativeTranslate.m128_f32[1]; up != 0.f)
	{
		transform._location = DirectX::XMVectorMultiplyAdd(
			transform._up,
			{ up, up, up, 0 },
			transform._location);
	}

	// relative translate
	if (const float forward = _relativeTranslate.m128_f32[2]; forward != 0.f)
	{
		transform._location = DirectX::XMVectorMultiplyAdd(
			transform._forward,
			{ forward, forward, forward, 0 },
			transform._location);
	}

	*_transformToModify = transform;
	Reset();
}