#pragma once

#include <cpprest/json.h>                       
#include <cpprest/uri.h> 
#include <cpprest/http_listener.h>
#include "../types.hpp"

namespace RS
{
	class RouteListener : public web::http::experimental::listener::http_listener
	{
	public:
		explicit RouteListener(std::string baseURL, std::string path, uint32_t verbs) :
			http_listener(utility::conversions::to_string_t(baseURL + path))
		{
			if (verbs & Verbs::GET)
				support(web::http::methods::GET, std::bind(&RouteListener::handle_get, this, std::placeholders::_1));
			if (verbs & Verbs::POST)
				support(web::http::methods::POST, std::bind(&RouteListener::handle_post, this, std::placeholders::_1));
			if (verbs & Verbs::PUT)
				support(web::http::methods::PUT, std::bind(&RouteListener::handle_put, this, std::placeholders::_1));
			if (verbs & Verbs::DEL)
				support(web::http::methods::DEL, std::bind(&RouteListener::handle_del, this, std::placeholders::_1));
		}
		virtual void handle_get(web::http::http_request message) {}
		virtual void handle_post(web::http::http_request message) {}
		virtual void handle_put(web::http::http_request message) {}
		virtual void handle_del(web::http::http_request message) {}
	};

	class BaseRouter
	{
		typedef std::vector<std::shared_ptr<RouteListener>> tlisteners;
	protected:
		BaseRouter(tlisteners listeners) : _listeners(listeners)
		{ }
	public:

		/** Starts all the listeners
		*/
		void start()
		{
			// Create a task group for a parallel run
			concurrency::task_group tg;

			// Invoke start on every listener
			std::for_each(_listeners.begin(), _listeners.end(), [&tg](auto listener) { tg.run([listener]() { listener->open().wait(); }); });

			// Wait for all of them to start
			tg.wait();
		}

		/** Stops all the listeners
		*/
		void stop()
		{
			// Create a task group for a parallel run
			concurrency::task_group tg;

			// Invoke stop on every listener
			std::for_each(_listeners.begin(), _listeners.end(), [&tg](auto listener) { tg.run([listener]() { listener->close().wait(); }); });

			// Wait for all of them to stop
			tg.wait();
		}
	private:
		tlisteners _listeners;
	};
}