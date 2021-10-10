#ifndef _TRAIT_H
#define _TRAIT_H

template<typename T>
struct function_traits_impl;

template<typename T>
struct function_traits :
    function_traits_impl<std::remove_cv_t<std::remove_reference_t<T>>> 
{};

template<typename Ret, typename ... Args>
struct function_traits_impl<Ret(Args...)> {
    using return_type = Ret;
    template<std::size_t I>
    struct args {
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };

    using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
};

template<typename Ret, typename ... Args>
struct function_traits_impl<Ret(*)(Args...)>: 
    function_traits_impl<Ret(Args...)> {};

template<typename Ret, typename Cls, typename ... Args>
struct function_traits_impl<Ret(Cls::*)(Args...)>: 
    function_traits_impl<Ret(Args...)> {};

template<typename func_type>
struct function_traits_args{
    using args_type = typename std::tuple_element<0,typename function_traits<func_type>::tuple_type>::type;
};

//类型帮助类，用于返回类型void的判断
template<typename T>
struct type_xx{
    using type = T;  
};

//特化
template<>
struct type_xx<void>{
    using type = int8_t;  
};

#endif
