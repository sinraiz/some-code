#pragma once
#include <string>
#include <ostream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>

namespace xdx
{
using logger_t = boost::log::sources::severity_logger<boost::log::trivial::severity_level>;
using logstream_t = boost::log::basic_record_ostream<char>;
class Log
{
public:
	static void init();
	static logger_t& get();
protected:
	static void formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm);
};
#define LOG(SEVERITY) BOOST_LOG_SEV(xdx::Log::get(), boost::log::trivial::SEVERITY)
}