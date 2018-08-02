#pragma once

#include <unordered_map>
#include <shared_mutex>
#include "../types.hpp"
#include "book.hpp"
#include "book-list.hpp"


namespace RS
{
	/**
	* Abstract books model (interface)
	*/
	class IBooks
	{
	public:
		/**
		* Adds the given book to collection.
		*
		* In case of an error throws rs_exception.
		*
		* @param book - The book to be added.
		* @returns A copy of the added book with the id assigned
		*/
		virtual Book create(const Book& book) = 0;

		/**
		* Updates the book identified by the given id
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be updated
		* @param book - The book's new data
		* @returns A copy of the updated book
		*/
		virtual Book update(tid id, const Book& book) = 0;

		/**
		* Deletes the book identified by the given id
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be updated
		*/
		virtual void remove(tid id) = 0;

		/**
		* Gets the book identified by the given id.
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be fetched
		* @returns A copy of the requested book
		*/
		virtual Book get(tid id) const = 0;

		/**
		* Gets the list of books contained in the model
		*
		* @param
		* @returns The list of all the books
		*/
		virtual JSONVector<Book> getAll() const = 0;
	};

	/**
	* This is a book model which stores the entire
	* collection in main memory.
	*/
	class BooksMemory : public IBooks
	{
		typedef std::unordered_map<tid, Book> BooksMap;
	public:
		/**
		* The default constructor. 
		* Sets the items ids sequence to 1.
		*/
		BooksMemory() :
			_next_id(0),
			_atomic_next_id(_next_id)
		{ }

		/**
		Copy constructor
		*/
		BooksMemory(const BooksMemory& _Right) :
			_items(_Right._items),
			_next_id(_Right._next_id),
			_atomic_next_id(_next_id)
		{ }

		/**
		Move constructor
		*/
		BooksMemory(BooksMemory&& _Right) noexcept :
			_items(std::move(_Right._items)),
			_next_id(_Right._next_id),
			_atomic_next_id(_next_id)
		{ }
	public: // IBookModel methods
		/**
		* Adds the given book to collection.
		*
		* In case of an error throws rs_exception.
		*
		* @param book - The book to be added.
		* @returns A copy of the added book with the id assigned
		*/
		virtual Book create(const Book& book) override
		{
			tid id = this->nextId();
			Book newBook(book);
			newBook.setId(id);

			// Acquire the writer's lock just for insert
			std::unique_lock<std::shared_mutex> lk(_lock);

			// Insert the new book
			_items[id] = newBook;

			// Return a copy
			return newBook;
		}

		/**
		* Updates the book identified by the given id
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be updated
		* @param book - The book's new data
		* @returns A copy of the updated book
		*/
		virtual Book update(tid id, const Book& book) override
		{
			// Acquire the writer's lock
			std::unique_lock<std::shared_mutex> lk(_lock);

			// Check if there's book with the given id 
			auto iBook = _items.find(id);
			if (iBook == _items.end())
			{
				throw rs_exception("Not found");
			}

			// Assign the fields
			iBook->second.setTitle(book.title());
			iBook->second.setPrice(book.price());

			// Return a copy
			return iBook->second;
		}

		/**
		* Deletes the book identified by the given id
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be updated
		*/
		virtual void remove(tid id) override
		{
			// Acquire the writer's lock
			std::unique_lock<std::shared_mutex> lk(_lock);

			// Check if there's book with the given id 
			auto iBook = _items.find(id);
			if (iBook == _items.end())
			{
				throw rs_exception("Not found");
			}

			// Delete
			_items.erase(iBook);
		}

		/**
		* Gets the book identified by the given id.
		*
		* In case of an error or if the book with such
		* id wasn't found the function throws rs_exception.
		*
		* @param id - The id of the book to be fetched
		* @returns A copy of the requested book
		*/
		virtual Book get(tid id) const override
		{
			// Acquire the reader's lock
			std::shared_lock<std::shared_mutex> lk(_lock);

			// Check if there's book with the given id 
			auto iBook = _items.find(id);
			if (iBook == _items.end())
			{
				throw rs_exception("Not found");
			}

			// Return a copy
			return iBook->second;
		}

		/**
		* Gets the list of books contained in the model
		*
		* @param
		* @returns The list of all the books
		*/
		virtual JSONVector<Book> getAll() const override
		{
			// Acquire the reader's lock
			std::shared_lock<std::shared_mutex> lk(_lock);

			// Prepare the resulting collection
			JSONVector<Book> result(_items.size());

			// Iterate the map and push them into array
			size_t i = 0;
			std::for_each(_items.begin(), _items.end(), [&result, &i](const BooksMap::value_type &elem) {
				result[i++] = elem.second;
			});

			return result;
		}

	protected:
		/**
		* Atomically increments the id sequence by 1
		* and returns the next sequence value
		* @returns The next id value to be used for a
		* new item in the collection.
		*/
		tid nextId()
		{
			return ++_atomic_next_id;
		}
	private:
		/**
		* The value used when assigning the id to a 
		* new item inserted into the collection.
		*/
		tid _next_id;

		/**
		* The atomic wrap used for the increments 
		* of the _next_id values
		*/
		std::atomic<tid> _atomic_next_id;

		/**
		* The mutex used to guard access to the collection
		*/
		mutable std::shared_mutex _lock;

		/**
		* The unordered collection of items
		*/
		BooksMap _items;
	};
}