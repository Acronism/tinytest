#pragma once

#include <stdio.h>

#include <string>
#include <map>
#include <functional>
#include <list>
#include <thread>
#include <unordered_map>
#include <deque>

#include "utility.h"

// CRAZY BEHIND-THE-SCENES SETUP STUFF

namespace TestDetail {

struct CheckFailureMessage {
	CheckFailureMessage(const char* format, int line, const char* extra = nullptr);

	const char* format; // literals only, doesn't get freed
	int line;
	const char* extra; // literals only, doesn't get freed
};

struct TestResults {
	TestResults();

	TestResults& operator+(const TestResults& right);
	TestResults& operator+=(const TestResults& right);

	unsigned int totalChecks;
	unsigned int failedChecks;
	unsigned int abortedTests;
	unsigned int abortedSuites;

	std::list<CheckFailureMessage> failureMessages;
};

struct TestSuite;

typedef std::function<void(TestResults*)> TestFunction;
typedef std::map<std::string, TestFunction> TestSuiteTestMap;

struct TestSuite {
	TestFunction setup;
	TestFunction teardown;

	TestSuiteTestMap tests;
	std::string name;

	bool run(TestResults* results);
};

enum TestException {
	TestExceptionAbortTest,
	TestExceptionAbortSuite,
	TestExceptionAbortAll,
};

typedef std::map<std::string, TestSuite> TestSuiteMap;

TestSuiteMap* GetTestSuiteMap();

class TestSuiteNamer {
	public:
		TestSuiteNamer(const char* name) {
			Name = name;
		}

		static const char* Name;
};

class TestSuiteCreator : public TestSuite {
	public:
		template <typename T>
		TestSuiteCreator(const T& function) {
			function(this);
			name = TestSuiteNamer::Name;
			(*GetTestSuiteMap())[TestSuiteNamer::Name] = *this;
		}
};

enum TestingOutputColor {
	kTestingOutputColorDefault,
	kTestingOutputColorGreen,
	kTestingOutputColorRed,
	kTestingOutputColorBlue,
};

class TestLogger {
public:
	// construction must not rely on any other global state

	// everything printed inside of a logging session is guaranteed to be contiguous
	void startSession();

	template <typename ... Args>
	void sessionWrite(std::string format, Args&& ... args) {
		{
			std::lock_guard<std::mutex> lk(_queueMutex);
			if (!_hasActive) {
				_hasActive = true;
				_activeSession = std::this_thread::get_id();
			}
			if (_activeSession != std::this_thread::get_id()) {
				_writeQueue[std::this_thread::get_id()].push_back(Format(std::move(format), std::forward<Args>(args)...));	
				return;
			}
		}
		write(std::move(format), std::forward<Args>(args)...); // this session is active
	}

	// ignores active session
	template <typename ... Args>
	void write(std::string format, Args&& ... args) {
		std::lock_guard<std::mutex> lk(_stdoutMutex);
		_doWrite(Format(std::move(format), std::forward<Args>(args)...));
	}

	void stopSession();

	// dumps the remaining logs to stdout in a thread-safe manner
	// necessary after all writes are finished
	void flush();
private:
	std::unordered_map<std::thread::id, std::deque<std::string>> _writeQueue;
	std::mutex _queueMutex;
	std::mutex _stdoutMutex;
	std::thread::id _activeSession; // guarded by _queueMutex
	bool _hasActive = false;        // guarded by _queueMutex

	// not thread safe
	void _SetTestingOutputColor(TestingOutputColor color);

	// not thread safe
	void _doWrite(const std::string& str);
};

extern TestLogger gTestLogger;

class TestLoggerSession {
public:
	inline TestLoggerSession() {
		gTestLogger.startSession();
	}
	inline ~TestLoggerSession() {
		gTestLogger.stopSession();
	}
};

// FUNCTIONS USED TO RUN TESTS

int RunTests(const char* suite = nullptr);

} // TestDetail

// MACROS USED TO CREATE TESTS

#define CONCATENATE_INNER(a, b) a ## b
#define CONCATENATE(a, b) CONCATENATE_INNER(a, b)

#define TEST_SUITE(name) \
	static TestDetail::TestSuiteNamer CONCATENATE(gTestSuiteNamer, __COUNTER__) = name; \
	static TestDetail::TestSuiteCreator CONCATENATE(gTestSuiteCreator, __COUNTER__) = [](TestDetail::TestSuite* suite)

#define SETUP() \
	suite->setup = [](TestDetail::TestResults* results)

#define TEARDOWN() \
	suite->teardown = [](TestDetail::TestResults* results)

#define TEST(name) \
	suite->tests[name] = [](TestDetail::TestResults* results)

#define ABORT_SUITE(...) \
	const char* format = "[RED]Abort Suite called in " __FILE__ " on line %d:\n    "; \
	TestDetail::gTestLogger.sessionWrite(format, __LINE__); \
	TestDetail::gTestLogger.sessionWrite("[RED]" __VA_ARGS__); \
	TestDetail::gTestLogger.sessionWrite("\n"); \
	results->failureMessages.push_back(TestDetail::CheckFailureMessage(format, __LINE__)); \
	throw TestDetail::TestExceptionAbortSuite;

#define ABORT_TEST() \
	const char* format = "[RED]Abort Test called in " __FILE__ " on line %d!\n"; \
	TestDetail::gTestLogger.sessionWrite(format, __LINE__); \
	results->failureMessages.push_back(TestDetail::CheckFailureMessage(format, __LINE__)); \
	throw TestDetail::TestExceptionAbortTest;

// exception != failed check
#define CHECK(condition) \
	++results->totalChecks; \
	if (!(condition)) { \
		++results->failedChecks; \
		const char* format = "[RED]Failed check in " __FILE__ " on line %d!\n"; \
		TestDetail::gTestLogger.sessionWrite(format, __LINE__); \
		results->failureMessages.push_back(TestDetail::CheckFailureMessage(format, __LINE__)); \
	}

// exception != failed check
#define CHECK_ESSENTIAL(condition) \
	++results->totalChecks; \
	if (!(condition)) { \
		++results->failedChecks; \
		const char* format = "[RED]Failed essential check in " __FILE__ " on line %d! Aborting test.\n"; \
		TestDetail::gTestLogger.sessionWrite(format, __LINE__); \
		results->failureMessages.push_back(TestDetail::CheckFailureMessage(format, __LINE__)); \
		throw TestDetail::TestExceptionAbortTest; \
	}

// these check macros don't evaluate true or false, they only check for the presence or absence of an exception

#define CHECK_EXCEPTION(expression, exception_type) \
	try { \
		(expression); \
		CHECK(false); \
	} catch (exception_type) { \
		CHECK(true); \
	}

#define CHECK_NO_EXCEPTION(expression) \
	try { \
		(expression); \
		CHECK(true); \
    } catch ( ... ) { \
		CHECK(false); \
	}

