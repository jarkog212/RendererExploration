#pragma once

#include <array>

namespace DX12::Math
{
	template <typename T>
	struct Base
	{
		Base() = default;
		Base(T o) : _data(o) {};
		Base(const Base<T>& o) : _data(o._data) {}
		Base<T>& operator=(const Base<T>& o) { _data = o._data; return *this; }
		Base(Base<T>&& o) : _data(std::move(o._data)) {}
		Base<T>& operator=(Base<T>&& o) { std::swap(_data, o._data); return *this; }

		T _data{};
	};

#pragma pack(push, 1)
	struct Vector2u32 : public Base<std::array<unsigned int, 2>>
	{
		Vector2u32(unsigned int X, unsigned int Y) { _data = { X, Y }; }
		unsigned int& X() { return _data[0]; }
		unsigned int X() const { return _data[0]; }
		unsigned int& Y() { return _data[1]; }
		unsigned int Y() const { return _data[1]; }
	};


	struct Vector3f : public Base<std::array<float, 3>> 
	{
		Vector3f(float X, float Y, float Z) { _data = { X, Y, Z}; }
		float& X() { return _data[0]; }
		float X() const { return _data[0]; }
		float& Y() { return _data[1]; }
		float Y() const { return _data[1]; }
		float& Z() { return _data[2]; }
		float Z() const { return _data[2]; }
	};
#pragma pack(pop)
}