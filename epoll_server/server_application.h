#pragma once

#include <vector>
#include <string>
#include <filesystem>


class ServerApplication
{
public:
	ServerApplication();
	ServerApplication(int argc, char* argv[]);
	~ServerApplication();

	using ArgVec = std::vector<std::string>;

	enum ExitCode
	{
		EXIT_OK = 0,  /// successful termination
		EXIT_USAGE = 64, /// command line usage error
		EXIT_DATAERR = 65, /// data format error
		EXIT_NOINPUT = 66, /// cannot open input
		EXIT_NOUSER = 67, /// addressee unknown
		EXIT_NOHOST = 68, /// host name unknown
		EXIT_UNAVAILABLE = 69, /// service unavailable
		EXIT_SOFTWARE = 70, /// internal software error
		EXIT_OSERR = 71, /// system error (e.g., can't fork)
		EXIT_OSFILE = 72, /// critical OS file missing
		EXIT_CANTCREAT = 73, /// can't create (user) output file
		EXIT_IOERR = 74, /// input/output error
		EXIT_TEMPFAIL = 75, /// temp failure; user is invited to retry
		EXIT_PROTOCOL = 76, /// remote error in protocol
		EXIT_NOPERM = 77, /// permission denied
		EXIT_CONFIG = 78  /// configuration error
	};

	enum ConfigPriority
	{
		PRIO_APPLICATION = -100,
		PRIO_DEFAULT = 0,
		PRIO_SYSTEM = 100
	};

	void init(const ArgVec& args);
	void init(int argc, char* argv[]);

	inline bool initialized() const;

	inline ServerApplication& instance();

	inline const ArgVec& argv() const;

	const std::chrono::steady_clock::time_point startTime() const;

	std::chrono::steady_clock::duration uptime() const;

public:
	bool isInteractive() const;

	int run(int argc, char** argv);

	int run(const std::vector<std::string>& args);

	virtual const char* name() const;

	static void terminate();

protected:
	void waitForTerminationRequest();

	bool isDaemon(int argc, char** argv);

	void beDaemon();

protected:
	void init();

	virtual void initialize(ServerApplication& self);

	virtual void uninitialize();

	virtual void reinitialize(ServerApplication& self);

protected:
	virtual int main(const std::vector<std::string>& args);

	int run();

private:
	void setup();

	void setArgs(int argc, char* argv[]);
	void setArgs(const ArgVec& args);

	void getApplicationPath(std::filesystem::path& appPath) const;

private:
	std::string _workingDirAtLaunch;
	std::string _command;
	ArgVec _argv;
	ArgVec _unprocessedArgs;

	bool _initialized;
	bool _unixOptions;

	std::chrono::steady_clock::time_point _startTime;

private:
	static ServerApplication* _pInstance;
};
