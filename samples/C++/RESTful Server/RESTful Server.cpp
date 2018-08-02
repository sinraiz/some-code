#include "stdafx.h"
#include "models/books.hpp"
#include "routers/helpers.hpp"

using namespace web::http;
using namespace web::json;
using namespace web::http::experimental::listener;

namespace RS
{	
	class VersionRoute : public RouteListener
	{
	public:
		using RouteListener::RouteListener;
		virtual void handle_get(http_request message) 
		{
			auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
			if (!paths.empty())
			{
				message.reply(status_codes::NotFound);
				return;
			}

			message.reply(status_codes::OK, "1.0");
		}
	};

	class BooksRoute : public RouteListener
	{
	public:
		using RouteListener::RouteListener;
		void handle_post(http_request message) override
		{
				auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
				if (!paths.empty())
				{
					message.reply(status_codes::NotFound);
					return;
				}
				BooksMemory &books = _books;
				message.extract_json()
					.then([&books, message](pplx::task<value> body) {
						try
						{
							message.reply(status_codes::OK, books.create(Book(body.get())).toJson());
						}
						catch (std::exception& ex)
						{
							message.reply(status_codes::BadRequest, ex.what());
							return;
						}
				});
		}
		void handle_get(http_request message) override
		{
			try
			{
				auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
				if (paths.empty())
				{
					message.reply(status_codes::OK, _books.getAll().toJson());
				}
				else if (paths.size() == 1)
				{
					int book_id = std::stoi(paths[0]);
					message.reply(status_codes::OK, _books.get(book_id).toJson());
				}
				else
				{
					message.reply(status_codes::NotFound);
					return;
				}
			}
			catch (std::exception& ex)
			{
				message.reply(status_codes::BadRequest, ex.what());
				return;
			}
		}
		void handle_del(http_request message) override
		{
			try
			{
				auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
				if (paths.size() != 1)
				{
					message.reply(status_codes::BadRequest);
					return;
				}
				
				int book_id = std::stoi(paths[0]);
				_books.remove(book_id);
				message.reply(status_codes::OK);
			}
			catch (std::exception& ex)
			{
				message.reply(status_codes::BadRequest, ex.what());
				return;
			}
		}
	private:
		BooksMemory _books;
	};

	class Router : public BaseRouter
	{
	public:
		Router(std::string baseURL) :
			BaseRouter({
				std::make_shared<VersionRoute>(baseURL, "/version", Verbs::GET),
				std::make_shared<BooksRoute>(baseURL, "/books", Verbs::GET | Verbs::POST | Verbs::DEL),
			})
		{}
	};
}

int main()
{
	std::string host_port = "127.0.0.1:3000";
	RS::Router r("http://" + host_port);

	std::cout << "Starting Web server..." << std::endl;
	r.start();
	std::cout << "Started at " << host_port << std::endl;

	while (!::_kbhit()) Sleep(100); 

	std::cout << "Stopping..." << std::endl;
	r.stop();
	std::cout << "Stopped" << std::endl;

    return 0;
}