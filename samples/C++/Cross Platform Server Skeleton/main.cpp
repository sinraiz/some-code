#include "pch.h"

#ifdef WIN32
#include "port/win32/service_setup.hpp"
#endif


using namespace std;
using namespace boost;
using namespace boost::application;
namespace po = boost::program_options;



namespace xdx
{

class Application
{
public:
	Application(context& context) :
		m_context(context)
	{
		LOG(trace) << "Application created";
	}
	~Application()
	{
		LOG(trace) << "Application cleaned up";
	}

	int operator()()
	{
		// your application logic here!
		// use ctrl to get state of your application...

		std::shared_ptr<run_mode> modes = m_context.find<run_mode>();

		if(modes->mode() == common::mode())
		{
			LOG(info) << "Started as an application";
		}
		else
		if(modes->mode() == server::mode())
		{
			LOG(info) << string("Started as a ") + _DAEMON_;
		}
		// application logic

		m_context.find<wait_for_termination_request>()->wait();

		return handleStop();
	}
	int handleStop()
	{
		LOG(info) << "Process being stopped...";
		
		LOG(info) << "Process stopped";
		return 0;
	}
	
	bool onServiceStop()
	{
		return true;
	}
protected:
	context& getContext(){ return m_context; }
private:
    context& m_context;
};

class AppSignalManager : public signal_manager
{
public:
	AppSignalManager(context& context)
		: signal_manager(context)
    {
        handler<>::callback cb = boost::bind<bool>(&AppSignalManager::stopSignal, this);
		LOG(info) << "Setting up the signal manager";

        boost::system::error_code ec[3];
		
      // define own signal handlers
#if defined( BOOST_WINDOWS_API )
        bind(SIGINT,  cb, ec[0]); // CTRL-C (2)
#elif defined( BOOST_POSIX_API )		
        bind(SIGUSR1, cb, ec[0]);
        bind(SIGUSR2, cb, ec[1]);	
        bind(SIGTERM, cb, ec[2]);
#endif
		if(ec[0] || ec[1] ||ec[2])
			LOG(info) << "Failed to setup the signal manager";
		else	
			LOG(info) << "The signal manager was setup";

    }

	bool stopSignal()
	{
		LOG(info) << "Termination was requested";
        // we need set application_state to stop
        context_.find<status>()->state(status::stopped);
        // and signalize wait_for_termination_request
        context_.find<wait_for_termination_request>()->proceed();
        return true;
   }
};

int parseCommandLine(int argc, char** argv, po::variables_map& cmd_args)
{
    po::options_description desc;
	desc.add_options()
		("help,h", "Show a help message")
		("daemon,d", "Run as a daemon (service)")
		;

	#ifdef WIN32
	desc.add_options()	
		("install,i", "Install service")
		("uninstall,u", "Unistall service")
		("name,n", po::value<std::string>()->default_value("Xedex"), "Service name")
		("display", po::value<std::string>()->default_value("Xedex"), "Service display name (optional, installation only)")
		("description", po::value<std::string>()->default_value("Xedex Universal Server"), "Service description (optional, installation only)")
		;
	#else
	desc.add_options();
	#endif

    // Parse
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc)
                    .allow_unregistered()
                    .run(), cmd_args);
        po::notify(cmd_args);
    }
    catch (std::exception ex)
	{
		cout << "Usage:" << endl <<
			desc << endl;
        return -1;
    }

    // Check if user needs help
    if (cmd_args.count("help"))// || argc == 1)
    {
        cout << "Usage:" << endl <<
			desc << endl;
        return 1;
    }
	else
	if (cmd_args.count("install")) // Install as a service
	{
		#ifdef WIN32
		cout << "Installing Windows service" << endl << Utils::getAppPath() << endl;
		boost::system::error_code ec;

		application::windows::install_windows_service(
			application::setup_arg(cmd_args["name"].as<std::string>()),
			application::setup_arg(cmd_args["display"].as<std::string>()),
			application::setup_arg(cmd_args["description"].as<std::string>()),
			application::setup_arg(Utils::getAppPath()),
			application::setup_arg(""), 
			application::setup_arg(""), 
			application::setup_arg("-d")).install(ec);

		std::cout << ec.message() << std::endl;

		return 1;
		#endif
	}
	else
	if (cmd_args.count("uninstall")) // Uninstall service
	{
		#ifdef WIN32
        cout << "Uninstalling Windows service" << endl;
		boost::system::error_code ec;
		application::windows::uninstall_windows_service(
			application::setup_arg(cmd_args["name"].as<std::string>()),
			application::setup_arg(Utils::getAppPath())).uninstall(ec);

		std::cout << ec.message() << std::endl;

		return 1;
		#endif
	}
	return 0;
}



void printBanner()
{
	cout << "XEDEX Server Node (" << sizeof(void*) * 8 << " bit)" << endl;
	cout << "Copyright 2014 by Xedex Software Solutions Ltd" << endl;
	cout << endl;
}

bool initConf()
{
	namespace fs = boost::filesystem;
	string appDir = xdx::Utils::getAppDir();
	fs::path confDir(xdx::Utils::getConfDir());
	if (!fs::exists(confDir))
	{
		cerr << "Configuration directory not found: " << confDir << endl;
		return false;
	}
	fs::path confPath = fs::absolute(fs::path("xdx.conf"), confDir);
	if (!fs::exists(confPath))
	{
		cerr << "Configuration file not found: " << confPath << endl;
		return false;
	}
	try
	{
		Config::get().load(confPath.string());
		Config::get().verify();
	}
	catch (std::exception ex)
	{
		cerr << "Failed to parse the configuration file: " << ex.what() << endl;
		return false;
	}
	return true;
}

bool initLogger()
{
	try
	{
		Log::init();
	}
	catch (std::exception ex)
	{
		cerr << "Failed to initilize the logger: " << ex.what() << endl;
		return false;
	}

	return true;
}
}

int main(int argc, char** argv)
{
	// Make a call to initialize and save the application path
	xdx::Utils::getAppPath(argv[0]);

	// Print the system information
	xdx::printBanner();

	
	if (!xdx::initConf() || // Read the config file
		!xdx::initLogger()  // Initialize the logger
	   )
	{
		LOG(fatal) << "Terminating...";
		return -1;
	}

	
	// Parse the command line
	po::variables_map cmd_args;
	int res = xdx::parseCommandLine(argc, argv, cmd_args);
	if (res < 0)
	{
	    LOG(fatal) << "Failed to parse the command line";
	    return -1;
	}
	else
	if (res > 0)
	{
		// Nothing to do, help message probably
		return 0;
	}
	
	
	context app_context;
	xdx::Application app(app_context);	
	
	// add the path information
	app_context.insert<path>( std::make_shared<path_default_behaviour>(argc, argv));
	
	// we will customize our signals behaviour
	xdx::AppSignalManager sm(app_context);
	
	int result = 0;
	boost::system::error_code ec;
	
	// we will run like a daemon or like a common application (foreground)
	if (cmd_args.count("daemon"))
	{
		//#ifdef WIN32 // add the service termination handler		
			application::handler<>::callback termination_callback = boost::bind<bool>(&xdx::Application::onServiceStop, &app);
			auto pTerminate = std::make_shared<application::termination_handler_default_behaviour>(termination_callback);
			app_context.insert<application::termination_handler>(pTerminate);
		//#endif
	    result = application::launch<server>(app, sm, app_context, ec);
	}
	else
	{
	    result = application::launch<common>(app, sm, app_context, ec);
	}
	
	// check for error
	
	if(ec)
	{
	    cout << "[E] " << ec.message() << " <" << ec.value() << "> " << std::endl;
	}

	return result;
}


