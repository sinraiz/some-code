#pragma once
#include <string>
#include <set>

namespace xdx
{
class Config
{
public:
	static Config& get()
	{
		static Config me;
		return me;
	}
	virtual ~Config();
	void load(const std::string &filename);
	void save(const std::string &filename);
	void verify();

	std::string getLogDir() { return m_logdir; }
	void setLogDir(const std::string& value) { m_logdir = value; }
protected:
	Config()
	{

	}
private:
	std::string m_logdir;          // log filename
	int m_loglevel;                // log level
	std::string m_datadir;         // log filename
	std::string m_dbdir;         // log filename
};

}