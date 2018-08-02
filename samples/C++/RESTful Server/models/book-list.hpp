#pragma once

#include <vector>
#include "book.hpp"

namespace RS
{
	/**
	* This class implements a vector of books that
	* additionally can serialize itself to JSON.
	*/
	class BookList : public std::vector<Book>
	{
	public:
		/**
		* Converts the collection of books to JSON array
		* @returns JSON array with objects representing each item
		*/
		web::json::value toJson() const
		{

		}
	};
}
