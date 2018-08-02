#pragma once

#include <string>
#include <algorithm>
#include <type_traits>
#include <cpprest/json.h>

namespace RS
{
	namespace Verbs
	{
		enum V : uint32_t
		{
			GET  = 1 << 0,
			POST = 1 << 1,
			PUT  = 1 << 2,
			DEL  = 1 << 3
		};
	}

	/**
	Just a handy alias to easily switch from UNICODE to ANSI
	and back
	*/
#ifdef _UNICODE
	typedef wchar_t tchar;
	typedef std::wstring tstring;
#else
	typedef char tchar;
	typedef std::string tstring;
#endif

	/**
	* The type is used for the internal ids
	*/
	typedef uint64_t tid;

	/**
	* A single exception type to represent errors in parsing, converting, and accessing
	* elements and data described by the models
	*/
	class rs_exception : public std::exception
	{
	public:
		rs_exception(const char * const &message) : std::exception(message)
		{ }
	};

	/**
	* Abstract class for types that can
	* convert their data to JSON
	*/
	class IJSONConvertible
	{
	public:
		virtual web::json::value toJson() const = 0;
	};


	/**
	* This class implements a vector that can
	* can convert itself to JSON provided that the 
	* type T of its elements is IJSONConvertible
	*/
	template<typename T> 
	class JSONVector : public std::vector<T>, public IJSONConvertible
	{
		static_assert(
			std::is_base_of<IJSONConvertible, T>::value,
			"T must be a descendant of IJSONConvertible"
			);
	public:
		explicit JSONVector(size_type _Count) : std::vector<T>(_Count)
		{ }

		/**
		* Converts the collection of items to JSON array
		* @returns JSON array with objects representing each item
		*/
		web::json::value toJson() const override
		{
			// Prepare an empty JSON array
			auto result = web::json::value::array();
			size_t n = 0;

			// Iterate the vector, convert each element to JSON
			// and add them to the resulting JSON array
			std::for_each(begin(), end(), [&result, &n](const T &elem) {
				result.as_array()[n++] = elem.toJson();
			});

			return result;
		}
	};
}
