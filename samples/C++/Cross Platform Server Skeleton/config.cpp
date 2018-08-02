#include "pch.h"
#include "config.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
namespace xdx
{
Config::~Config()
{

}
void Config::load(const std::string &filename)
{
	// Refer to: http://www.boost.org/doc/libs/1_42_0/doc/html/boost_propertytree/tutorial.html
	// Create an empty property tree object
	using boost::property_tree::ptree;
	ptree pt;

	// Load the XML file into the property tree. If reading fails
	// (cannot open file, parse error), an exception is thrown.
	read_xml(filename, pt);

	// Read the preferred directories configuration
	m_datadir = pt.get<std::string>("directories.data");
	m_dbdir = pt.get<std::string>("directories.db");
	m_logdir = pt.get<std::string>("directories.logs");

	m_loglevel = pt.get("log.level", 0);
}
void Config::save(const std::string &filename)
{
	// Create an empty property tree object
	using boost::property_tree::ptree;
	ptree pt;

	// Save the directories info
	pt.put("directories.data", m_datadir);
	pt.put("directories.db", m_dbdir);
	pt.put("directories.logs", m_logdir);

	// Save logging parameters
	pt.put("log.level", m_loglevel);

	// Write the property tree to the XML file.
	write_xml(filename, pt);
}

void Config::verify()
{
	bool isValid = false;

	string logsDir = xdx::Utils::resolveDir(m_logdir, isValid);
	if (!isValid)
		throw runtime_error(string("Failed to locate the logs directory: ") + logsDir);
	string dataDir = xdx::Utils::resolveDir(m_datadir, isValid);
	if (!isValid)
		throw runtime_error(string("Failed to locate the data directory: ") + dataDir);
	string dbDir = xdx::Utils::resolveDir(m_dbdir, isValid);
	if (!isValid)
		throw runtime_error(string("Failed to locate the database directory: ") + dbDir);
}
}