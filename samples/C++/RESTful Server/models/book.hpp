#pragma once

#include <tchar.h>
#include <cpprest/json.h>
#include "../types.hpp"

namespace RS
{
	/**
	* This is class represents one piece of
	* data that we store in the book models.
	* Currently it only carries the book's
	* title and its price in the store.
	*/
	class Book : public IJSONConvertible
	{
		/**
		* The books storage should be able to assign the book's id
		*/
		friend class BooksMemory;
	public:
		/**
		* The default constructor. Nothing much here, just
		* some readable values
		*/
		Book() :
			_id(0),
			_title(_TEXT("Undefined")),
			_price(0.0)
		{ }

		/**
		* Copy constructor
		*/
		Book(const Book& book) :
			_id(book._id),
			_title(book._title),
			_price(book._price)
		{ }

		/**
		* Assignment
		*/
		Book& operator=(Book const& book) 
		{ 
			this->_id = book._id;
			this->_title = book._title;
			this->_price = book._price;

			return *this; 
		}

		/**
		* Move constructor
		*/
		Book(Book&& book) noexcept :
			_id(book._id),
			_title(std::move(book._title)),
			_price(book._price)
		{ }

		/**
		* The constructor copying values from the given JSON
		* object.
		* If the given JSON value is not an object or if any
		* of the book's mandatory fields are not present in JSON
		* an rs_exception is thrown.
		*
		* @param jsonValue A JSON object to copy the values from
		*/
		Book(const web::json::value& jsonValue) : Book()
		{
			// Validate the value and cast to JSON object
			if (!jsonValue.is_object())
			{
				throw rs_exception("Not an object");
			}
			auto jsonObject = jsonValue.as_object();

			// Validate that fields exist and have the correct types
			try
			{
				// We'll not deserialize the id from JSON as it's passed
				// with URLs
				// ...

				// Make sure that the manadatory fields are there
				auto titleValue = jsonObject.at(_T("title"));
				auto priceValue = jsonObject.at(_T("price"));

				// Check if they have the correct types
				if (!titleValue.is_string() || !priceValue.is_double())
				{
					throw web::json::json_exception(_T("Invalid field types"));
				}

				// Read the values and assign
				this->setTitle(titleValue.as_string());
				this->setPrice(priceValue.as_double());
			}
			catch (const web::json::json_exception&)
			{
				throw rs_exception("Invalid format");
			}
		}

		/**
		* Converts this book to JSON
		*/
		web::json::value toJson() const override
		{
			// Prepare an empty JSON object
			auto result = web::json::value::object();

			// Add the attributes and set their values
			result[_T("id")] = web::json::value::number(this->_id);
			result[_T("title")] = web::json::value::string(this->_title);
			result[_T("price")] = web::json::value::number(this->_price);

			return result;
		}

		#pragma region Properties
	
		/**
		* Gets the the book's internal id
		*/
		tid id() const
		{
			return this->_id;
		}

		/**
		* Gets the the book's title attribute's value
		* @returns value of the book's title attribute
		*/
		const tstring& title() const
		{
			return this->_title;
		}

		/**
		* Sets the the book's title attribute's value
		* @param value The new title to put
		*/
		void setTitle(const tstring& value)
		{
			if (value.length() == 0)
			{
				throw rs_exception("Invalid title");
			}
			this->_title = value;
		}

		/**
		* Gets the the book's price attribute's value
		* @returns value of the book's price attribute
		*/
		double price() const
		{
			return this->_price;
		}

		/**
		* Sets the the book's price attribute's value
		* @param value The new price to put
		*/
		void setPrice(double value)
		{
			if (value < 0)
			{
				throw rs_exception("Invalid price");
			}
			this->_price = value;
		}
		
		#pragma endregion
	protected:
		/**
		* Sets the the book's id. 
		* It shouldn't be called from outside the models
		*/
		virtual void setId(tid value)
		{
			if (value <= 0)
			{
				throw rs_exception("Invalid id");
			}
			this->_id = value;
		}
	private:
		/**
		* The book's internal ID
		*/
		tid _id;

		/**
		* Stores the book's title
		*/
		tstring _title;

		/**
		* The book's price
		*/
		double _price;
	};
}