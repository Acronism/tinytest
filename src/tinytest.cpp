#include "tinytest.h"

// lets exceptions through so they can be caught by the debugger
// #define DEBUG_EXCEPTIONS

#ifdef _WIN32
#include <Windows.h>
#endif

namespace TestDetail {

TestLogger gTestLogger;

// everything printed inside of a logging session is guaranteed to be contiguous
void TestLogger::startSession() {
	std::lock_guard<std::mutex> lk(_queueMutex);
	if (!_hasActive) {
		_hasActive = true;
		_activeSession = std::this_thread::get_id();
	}
	// TODO: don't append sessions on older sessions?
}

void TestLogger::stopSession() {
	std::lock_guard<std::mutex> lk(_queueMutex);
	if (_hasActive && _activeSession == std::this_thread::get_id()) {
		// cycle to a non-empty queue, if there is no non-empty queue, there is no active session
		_hasActive = false;
		for (auto&& qPair : _writeQueue) {
			if (!qPair.second.empty()) {
				_hasActive = true;
				_activeSession = qPair.first;
				break;
			}
		}
		if (_hasActive) {
			while (!_writeQueue[_activeSession].empty()) {
				_doWrite(_writeQueue[_activeSession].front());
				_writeQueue[_activeSession].pop_front();
			}
		}
	}
}

void TestLogger::flush() {
	std::lock(_queueMutex, _stdoutMutex);
	std::unique_lock<std::mutex> lk1(_queueMutex, std::adopt_lock);
	std::unique_lock<std::mutex> lk2(_stdoutMutex, std::adopt_lock);

	for (auto&& qPair : _writeQueue) {
		while (!qPair.second.empty()) {
			_doWrite(qPair.second.front());
			qPair.second.pop_front();
		}
	}
}

void TestLogger::_doWrite(const std::string& str) {
	auto it = str.begin();
	while (it != str.end()) {
		auto start = std::find(it, str.end(), '[');
		auto end = std::find(start, str.end(), ']');

		if (it != start) {
			std::cout.write(str.c_str() + std::distance(str.begin(), it), std::distance(it, start));
		}

		if (start == str.end()) {
			break;
		}
		if (end != str.end()) {
			std::string tag(start, end + 1);
			if (tag == "[RED]") {
				_SetTestingOutputColor(kTestingOutputColorRed);
			} else if (tag == "[GREEN]") {
				_SetTestingOutputColor(kTestingOutputColorGreen);
			} else if (tag == "[BLUE]") {
				_SetTestingOutputColor(kTestingOutputColorBlue);
			} else if (tag == "[DEFAULT]") {
				_SetTestingOutputColor(kTestingOutputColorDefault);
			} else {
				// tag not matched, print it as normal text
				std::cout.write(str.c_str() + std::distance(str.begin(), start), std::distance(start, end));
			}
		}

		it = end + 1;
	}
	_SetTestingOutputColor(kTestingOutputColorDefault);
}

void TestLogger::_SetTestingOutputColor(TestingOutputColor color) {
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	static bool shouldFetchDefault = true;
	static WORD defaultAttribute = 15;

	if (shouldFetchDefault) {
		CONSOLE_SCREEN_BUFFER_INFO info;
		if (GetConsoleScreenBufferInfo(hConsole, &info)) {
			defaultAttribute = info.wAttributes;
		}
		shouldFetchDefault = false;
	}

	switch (color) {
		case kTestingOutputColorGreen:
			SetConsoleTextAttribute(hConsole, (defaultAttribute & 0xFFF0) | FOREGROUND_GREEN);
			break;
		case kTestingOutputColorRed:
			SetConsoleTextAttribute(hConsole, (defaultAttribute & 0xFFF0) | FOREGROUND_RED);
			break;
		case kTestingOutputColorBlue:
			SetConsoleTextAttribute(hConsole, (defaultAttribute & 0xFFF0) | FOREGROUND_BLUE | FOREGROUND_GREEN);
			break;
		default:
			SetConsoleTextAttribute(hConsole, defaultAttribute);
			break;
	}
#else
	switch (color) {
		case kTestingOutputColorGreen:
			printf("\033[22;32m");
			break;
		case kTestingOutputColorRed:
			printf("\033[22;31m");
			break;
		case kTestingOutputColorBlue:
			printf("\033[22;34m");
			break;
		default:
			printf("\033[22;0m");
			break;
	}
#endif
}

CheckFailureMessage::CheckFailureMessage(const char* format, int line, const char* extra) : format(format), line(line), extra(extra) {
}

TestResults::TestResults()
	: totalChecks(0)
	, failedChecks(0)
	, abortedTests(0)
	, abortedSuites(0)
{}

TestResults& TestResults::operator+(const TestResults& right) {
	totalChecks  += right.totalChecks;
	failedChecks += right.failedChecks;
	abortedTests += right.abortedTests;
	abortedSuites += right.abortedSuites;
	for (const CheckFailureMessage& m : right.failureMessages) {
		failureMessages.push_back(m);
	}
	return *this;
}

TestResults& TestResults::operator+=(const TestResults& right) {
	return (*this = *this + right);
}

bool TestSuite::run(TestResults* results) {
	TestLoggerSession session;

	gTestLogger.sessionWrite("===========================================================\n"
	                       "[BLUE]Running test suite %s...\n", name.c_str());
	TestResults r;
	bool suiteFailed = false;
	bool abortAll = false;

	// setup
	try {
		if (setup) {
			gTestLogger.sessionWrite("Setting up...\n");
			setup(&r);
		}
	} catch (const TestException) {
		suiteFailed = true;
#ifndef DEBUG_EXCEPTIONS
	} catch (...) {
		gTestLogger.sessionWrite("[RED]Unhandled exception thrown during setup for '%s' suite.\n", name.c_str());
		suiteFailed = true;
#endif
	}

	// tests
	if (!suiteFailed) {
		for (auto it = tests.begin(); it != tests.end(); ++it) {
			try { // try each test
				gTestLogger.sessionWrite("Testing %s...\n", it->first.c_str());
				it->second(&r);
			} catch (const TestException& e) {
				if (e == TestExceptionAbortSuite) {
					++r.abortedSuites;
					suiteFailed = true;
					break;
				}
				if (e == TestExceptionAbortTest) {
					++r.abortedTests;
					// do nothing, run the remaining tests
				}
#ifndef DEBUG_EXCEPTIONS
			} catch (...) {
				// TODO: it'd probably be safe to just abort the test
				gTestLogger.sessionWrite("[RED]Unhandled exception thrown during '%s' test. Aborting suite.\n", it->first.c_str());
				++r.abortedSuites;
				suiteFailed = true;
				break;
#endif
			}
		}
	}

	// teardown
#ifndef DEBUG_EXCEPTIONS
	try {
#endif
		if (teardown) {
			gTestLogger.sessionWrite("Tearing down...\n");
			teardown(&r);
		}
		if (!r.failedChecks) {
			gTestLogger.sessionWrite("[GREEN]All %u checks passed!\n", r.totalChecks);
		} else {
			gTestLogger.sessionWrite("[RED]%u of %u checks failed!\n", r.failedChecks, r.totalChecks);
		}
#ifndef DEBUG_EXCEPTIONS
	} catch (...) {
		gTestLogger.sessionWrite("[RED]Unhandled exception thrown during teardown for '%s' suite. Aborting ALL suites.\n", name.c_str());
		abortAll = true;
	}
#endif

	*results += r;

	if (abortAll) {
		throw TestExceptionAbortAll;
	}

	return !suiteFailed;
}

int RunTests(const char* suite) {

	std::map<std::string, TestResults> results;
	TestResults overall;

	size_t completedSuites = 0;

	if (suite) {
		auto it = GetTestSuiteMap()->find(suite);
		if (it == GetTestSuiteMap()->end()) {
			gTestLogger.write("[RED]Test suite \"%s\" not found.\n", suite);
		} else {
			TestResults r;
			try {
				if (it->second.run(&r)) {
					++completedSuites;
				}
			} catch (TestException) {
				// do nothing
			}
			results[suite] = r;
			overall += r;
		}
	} else {
		// would be much easier with boost asio, but that requires a library
		std::vector<std::thread> threads;
		std::mutex mapMutex;
		const size_t concurrency = std::thread::hardware_concurrency();

		for (auto it = GetTestSuiteMap()->begin(); it != GetTestSuiteMap()->end(); ++it) {
			if (threads.size() == concurrency) {
				// concurrency capped, wait for a thread to finish
				while (threads.size() == concurrency) {
					for (auto it = threads.begin(); it != threads.end();) {
						if (it->joinable()) {
							it->join();
							it = threads.erase(it);
						} else {
							++it;
						}
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			threads.emplace_back([&, it]{
				TestResults r;
				try {
					if (it->second.run(&r)) {
						++completedSuites;
					}
				} catch (TestException) {}
				{
					std::lock_guard<std::mutex> lk(mapMutex);
					results[it->first] = r;
				}
				overall += r;
			});
		}

		while (!threads.empty()) {
			if (threads.back().joinable()) {
				threads.back().join();
			}
			threads.pop_back();
		}
		gTestLogger.flush();
	}

	size_t incompletedSuites = (suite ? 1 : GetTestSuiteMap()->size()) - completedSuites;

	gTestLogger.write("========================= SUMMARY =========================\n");
	if (incompletedSuites) {
		gTestLogger.write("[RED]Warning: One or more suites were not completed. Results may be incomplete.\n");
	}
	gTestLogger.write("Total checks performed: %d\n", overall.totalChecks);
	if (!overall.failedChecks) {
		gTestLogger.write("[GREEN]Total checks failed: %d\n", overall.failedChecks);
	} else {
		gTestLogger.write("[RED]Total checks failed: %d\n", overall.failedChecks);
	}
	if (overall.abortedTests) {
		gTestLogger.write("[RED]Aborted tests: %d\n", overall.abortedTests);
	}
	if (overall.abortedSuites) {
		gTestLogger.write("[RED]Aborted suites: %d\n", overall.abortedSuites);
	}
	for (auto it = results.begin(); it != results.end(); ++it) {
		if (it->second.failedChecks == 0) {
			continue;
		}
		gTestLogger.write("-----------------------------------------------------------\n");
		gTestLogger.write("[RED]%s: %d %s\n", it->first.c_str(), it->second.failedChecks, it->second.failedChecks == 1 ? "failure" : "failures");
		for (const CheckFailureMessage& m : it->second.failureMessages) {
			if (m.extra) {
				gTestLogger.write(m.format, m.line, m.extra);
			} else {
				gTestLogger.write(m.format, m.line);
			}
		}
	}
	gTestLogger.write("===========================================================\n");

	return incompletedSuites + overall.failedChecks + overall.abortedTests + overall.abortedSuites;
}

const char* TestSuiteNamer::Name;

TestSuiteMap* GetTestSuiteMap() {
	static TestSuiteMap* suites = new TestSuiteMap();
	return suites;
}

} // TestDetail

int main(int argc, char* argv[]) {
	int ret = 0;

	if (argc >= 2) {
		for (int i = 1; i < argc; ++i) {
			ret += TestDetail::RunTests(argv[i]);
		}
	} else {
		ret = TestDetail::RunTests();
	}

	return ret;
}