#pragma once
#include "DirectXMath.h"

template <typename D>
class AttributeHolder
{
public:

	virtual ~AttributeHolder() { DEBUG_DESTRUCTOR_BODY(AttributeHolder); }

	template<typename T>
	typename T::AttributeType* GetAttribute()
	{
		T* asType = dynamic_cast<T*>(static_cast<D*>(this));
		return asType ? asType->GetValue() : nullptr;
	}
};

class ATransform 
{
public:

	virtual ~ATransform() { DEBUG_DESTRUCTOR_BODY(ATransform); }

	struct Transform
	{
		DirectX::XMVECTOR _location{};
		DirectX::XMVECTOR _direction{};
		DirectX::XMVECTOR _scale{};
	};

	using AttributeType = Transform;

	AttributeType* GetValue() { return &_transform; };

protected:

	Transform _transform;
};

class AMovable
{
public:

	AMovable(ATransform* self) { _transformToModify = self->GetValue(); }
	virtual ~AMovable() { DEBUG_DESTRUCTOR_BODY(AMovable); }

	void Reset() { /*TODO*/ };
	void Apply() { /*TODO*/ };
	void Forward(float) { /*TODO*/ };
	void Right(float) { /*TODO*/ };
	void Turn(DirectX::XMFLOAT3) { /*TODO*/ };
	
	using AttributeType = AMovable;

	AttributeType* GetValue() { return this; };

protected:

	float _forward = 0;
	float _right = 0;
	DirectX::XMVECTOR _rotate{};
	ATransform::Transform* _transformToModify = nullptr;
};

class AIUpdatable
{
public:

	virtual ~AIUpdatable() { DEBUG_DESTRUCTOR_BODY(AUpdatable); }
	
	using AttributeType = AIUpdatable;

	AttributeType* GetValue() { return this; };

	virtual void Update(float deltaTime) = 0;
};
