#pragma once
#include <string>
#include <boost/filesystem.hpp>

namespace xdx
{
	class Utils
	{
	public:
		static std::string getAppPath(std::string cmdLine = "")
		{
			namespace fs = boost::filesystem;
		#ifdef WIN32
			// Windows has a bad thing that in command line you are allowed to ommit the 
			// .exe from the executable name, so we'll have to do it the windows way
			char buffer[MAX_PATH];
			GetModuleFileName(NULL, buffer, MAX_PATH);
			static std::string appPath = buffer;
		#else
			static std::string appPath = fs::system_complete(fs::path(cmdLine)).string();
		#endif
			return appPath;
		}
		static std::string getAppDir()
		{
			namespace fs = boost::filesystem;
			fs::path fullAppPath(getAppPath());
			return fullAppPath.parent_path().string();
		}
		static std::string resolveDir(const std::string& dir, bool& valid)
		{
			namespace fs = boost::filesystem;

			std::string result = fs::absolute(dir, getAppDir()).string();
			valid = fs::exists(result);
			return result;
		}
		static std::string resolveDir(const std::string& dir)
		{
			bool valid = false;
			return resolveDir(dir, valid);
		}
		static std::string getConfDir()
		{
			return resolveDir("conf");
		}
		static std::string getDataPath(std::string firstTime = "")
		{
			namespace fs = boost::filesystem;
			fs::path appDir(getAppDir());

			std::string checkPath = "";
			try
			{
				checkPath = fs::canonical(fs::path(firstTime), appDir).string();
			}
			catch (fs::filesystem_error  ex)
			{
				return "";
			}

			static std::string dataPath = checkPath;
			return dataPath;
		}
	};
}