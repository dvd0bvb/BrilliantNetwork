#pragma once

#include <type_traits>

namespace Brilliant
{
    template<class T, class R = void>
    struct enable_if_type { using type = R; };

    template<class T, class U, class Enable = void>
    struct has_type : public std::false_type {};

    template<class T, class U>
    struct has_type<T, U, typename enable_if_type<typename T::U>::type> : std::true_type {};

    template<class T, class... Us>
    struct has_types : public std::integral_constant<bool, (has_type<T, Us>::value && ...)> {};

    template<class T, class U>
    concept HasType = has_type<T, U>::value;

    template<class T, class... Us>
    concept HasTypes = has_types<T, Us...>::value;

}