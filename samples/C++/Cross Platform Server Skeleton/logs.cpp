#include "pch.h"
#include "logs.hpp"
#include "config.hpp"
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

using namespace std;

namespace xdx
{
void Log::init()
{
	string logsDir = Config::get().getLogDir();
	string logsDirFull = xdx::Utils::resolveDir(logsDir);

	auto fileLogger = logging::add_file_log
	(
		keywords::target = logsDirFull,
		keywords::file_name = logsDirFull + "/%Y%m%d.log",
		//keywords::rotation_size = 1, //10 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
		keywords::auto_flush = true,
        keywords::open_mode = (std::ios::out | std::ios::app)
	);
	fileLogger->set_formatter(&formatter);

	

	// Add the sink to the core
	logging::core::get()->add_sink(fileLogger);

	auto consoleLogger = logging::add_console_log
	(
		std::cout	
	);
	consoleLogger->set_formatter(&formatter);

//
	logging::core::get()->set_filter
	(
		logging::trivial::severity >= logging::trivial::info
	);
	
	logging::add_common_attributes();
}

logger_t& Log::get()
{
	static logger_t lg;
	return lg;
}
void Log::formatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
{
	logging::trivial::severity_level sevLevel = *rec[logging::trivial::severity];
	static const char* strings[] =
	{
		"TRAC",
		"DBUG",
		"INFO",
		"WARN",
		"ERRR",
		"CRIT"
	};    
	auto date_time_formatter = expr::stream << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S");
	date_time_formatter(rec, strm);
	strm << std::setw(10);
	
	if (static_cast< std::size_t >(sevLevel) < sizeof(strings) / sizeof(*strings))
		strm << std::setw(6) << strings[sevLevel];
	else
		strm << std::setw(6) << static_cast< int >(sevLevel);

	strm << std::setw(2) << " " << rec[expr::message];

}
}