#pragma once

#include <type_traits>

namespace Brilliant
{
	template<class T>
	concept EnumType = std::is_enum_v<T>; 

	template<class T>
	concept TriviallyCopiable = std::is_trivially_copyable_v<T>;
}