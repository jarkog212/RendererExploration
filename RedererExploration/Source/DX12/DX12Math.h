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
		Base<T>& operator=(Base<T>&& o) { _data = std::move(o._data); return *this; }

		T _data{};
	};

	struct Vector2u32 : public Base<std::array<unsigned int, 2>>
	{
		Vector2u32(unsigned int X, unsigned int Y) { _data = { X, Y }; }
		unsigned int& X() { return _data[0]; }
		unsigned int& Y() { return _data[1]; }
	};
}