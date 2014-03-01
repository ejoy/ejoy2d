#pragma once

#include <typeinfo>

namespace msvc { namespace type {

	struct empty_t {};
	
	template <const std::type_info& type_id>
	struct type_id {};
	
	template <typename ID, typename T = empty_t>
	struct extract_t;
	
	template <typename ID>
	struct extract_t<ID, empty_t>
	{
		template <bool>
		struct id2type;
	};
	
	template <typename ID, typename T>
	struct extract_t : extract_t<ID, empty_t>
	{
		template <>
		struct id2type<true> { typedef T type; };
	};
	
	template <typename T, typename ID>
	struct register_t : extract_t<ID, T>
	{
		typedef typename id2type<true>::type type;
	};
	
	template <typename T>
	struct encode_t
	{
		typedef typename register_t<T*, type_id<typeid(T*)>>::type type;
	};
	
	template <typename T>
	typename encode_t<T>::type encode(const T&);
	template <typename T>
	typename encode_t<T>::type encode(T&);
	
	template <typename T>
	struct decode_t;
	template <typename T>
	struct decode_t<T*> { typedef T type; };

#define typeof(...) \
	msvc::type::decode_t< \
		msvc::type::extract_t< \
			msvc::type::type_id<typeid(msvc::type::encode(__VA_ARGS__))> \
		>::id2type<true>::type \
	>::type

}}
