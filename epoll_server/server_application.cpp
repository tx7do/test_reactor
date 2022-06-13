#include "server_application.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>

ServerApplication* ServerApplication::_pInstance = nullptr;

ServerApplication::ServerApplication()
	: _initialized(false), _unixOptions(true)
{
	setup();
}

ServerApplication::ServerApplication(int argc, char* argv[])
	: _initialized(false), _unixOptions(true)
{
	setup();
	init(argc, argv);
}

ServerApplication::~ServerApplication()
{
	_pInstance = nullptr;
}

void ServerApplication::terminate()
{
	kill(getpid(), SIGINT);
}

void ServerApplication::setup()
{
	_pInstance = this;
}

void ServerApplication::init(int argc, char* argv[])
{
	setArgs(argc, argv);
	init();
}

void ServerApplication::init(const ArgVec& args)
{
	setArgs(args);
	init();
}

void ServerApplication::init()
{
	std::filesystem::path appPath;
	getApplicationPath(appPath);
}

const char* ServerApplication::name() const
{
	return "ServerApplication";
}

void ServerApplication::initialize(ServerApplication& self)
{
	_initialized = true;
}

void ServerApplication::uninitialize()
{
	if (_initialized)
	{
		_initialized = false;
	}
}

void ServerApplication::reinitialize(ServerApplication& self)
{
}

void ServerApplication::waitForTerminationRequest()
{
	sigset_t sset;
	sigemptyset(&sset);
	if (!std::getenv("ENABLE_DEBUGGER"))
	{
		sigaddset(&sset, SIGINT);
	}
	sigaddset(&sset, SIGQUIT);
	sigaddset(&sset, SIGTERM);
	sigprocmask(SIG_BLOCK, &sset, NULL);
	int sig;
	sigwait(&sset, &sig);
}

int ServerApplication::run(int argc, char** argv)
{
	bool runAsDaemon = isDaemon(argc, argv);
	if (runAsDaemon)
	{
		beDaemon();
	}
	try
	{
		init(argc, argv);
		if (runAsDaemon)
		{
			int rc = chdir("/");
			if (rc != 0) return EXIT_OSERR;
		}
	}
	catch (std::exception& exc)
	{
		return EXIT_CONFIG;
	}
	return run();
}

int ServerApplication::run(const std::vector<std::string>& args)
{
	bool runAsDaemon = false;
	for (const auto& arg : args)
	{
		if (arg == "--daemon")
		{
			runAsDaemon = true;
			break;
		}
	}
	if (runAsDaemon)
	{
		beDaemon();
	}
	try
	{
		init(args);
		if (runAsDaemon)
		{
			int rc = chdir("/");
			if (rc != 0) return EXIT_OSERR;
		}
	}
	catch (std::exception& exc)
	{
		return EXIT_CONFIG;
	}
	return run();
}

int ServerApplication::run()
{
	int rc = EXIT_CONFIG;

	try
	{
		initialize(*this);
		rc = EXIT_SOFTWARE;
		rc = main(_unprocessedArgs);
	}
	catch (std::exception& exc)
	{
		std::cerr << "Exception: " << exc.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown exception" << std::endl;
	}

	uninitialize();
	return rc;
}

int ServerApplication::main(const ArgVec& args)
{
	return EXIT_OK;
}

void ServerApplication::setArgs(int argc, char* argv[])
{
	_command = argv[0];
	_unprocessedArgs.reserve(argc);
	for (int i = 0; i < argc; ++i)
	{
		std::string arg(argv[i]);
		_unprocessedArgs.push_back(arg);
	}
}

void ServerApplication::setArgs(const ArgVec& args)
{
	_command = args[0];
	_unprocessedArgs = args;
}

void ServerApplication::getApplicationPath(std::filesystem::path& appPath) const
{
	if (_command.find('/') != std::string::npos)
	{
		std::filesystem::path path(_command);
		if (path.is_absolute())
		{
			appPath = path;
		}
		else
		{
			appPath = _workingDirAtLaunch;
			appPath += path;
		}
	}
	else
	{
		appPath = std::filesystem::current_path();
		if (appPath.empty())
		{
			appPath = std::filesystem::path(_workingDirAtLaunch + _command);
		}
	}
}

bool ServerApplication::isDaemon(int argc, char** argv)
{
	std::string option("--daemon");
	for (int i = 1; i < argc; ++i)
	{
		if (option == argv[i])
			return true;
	}
	return false;
}

void ServerApplication::beDaemon()
{
	pid_t pid;
	if ((pid = fork()) < 0)
	{
		throw std::runtime_error("Fork failed");
	}
	else if (pid != 0)
	{
		exit(EXIT_OK);
	}

	setsid();
	umask(027);

	FILE* fin = freopen("/dev/null", "r+", stdin);
	if (!fin) throw std::runtime_error("Failed to redirect stdin");
	FILE* fout = freopen("/dev/null", "r+", stdout);
	if (!fout) throw std::runtime_error("Cannot attach stdout to /dev/null");
	FILE* ferr = freopen("/dev/null", "r+", stderr);
	if (!ferr) throw std::runtime_error("Cannot attach stderr to /dev/null");
}

bool ServerApplication::isInteractive() const
{
	return false;
}

const std::chrono::steady_clock::time_point ServerApplication::startTime() const
{
	return _startTime;
}

std::chrono::steady_clock::duration ServerApplication::uptime() const
{
	auto now = std::chrono::steady_clock::now();
	auto uptime = now - _startTime;
	return uptime;
}

const ServerApplication::ArgVec& ServerApplication::argv() const
{
	return _argv;
}

ServerApplication& ServerApplication::instance()
{
	return *_pInstance;
}

bool ServerApplication::initialized() const
{
	return _initialized;
}
