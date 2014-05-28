#include "tinytest.h"

// lets exceptions through so they can be caught by the debugger
// #define DEBUG_EXCEPTIONS

#ifdef _WIN32
#include <Windows.h>
#endif

namespace TestDetail {

#ifdef _WIN32
void SetTestingOutputColor(TestingOutputColor color) {
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
}
#else
void SetTestingOutputColor(TestingOutputColor color) {
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
}
#endif

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
	SetTestingOutputColor(kTestingOutputColorDefault);

	printf("===========================================================\n");
	SetTestingOutputColor(kTestingOutputColorBlue);
	printf("Running test suite %s...\n", name.c_str());
	SetTestingOutputColor(kTestingOutputColorDefault);
	TestResults r;
	bool suiteFailed = false;
	bool abortAll = false;

	// setup
	try {
		if (setup) {
			printf("Setting up...\n");
			setup(&r);
		}
	} catch (const TestException) {
		suiteFailed = true;
#ifndef DEBUG_EXCEPTIONS
	} catch (...) {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Unhandled exception thrown during setup for '%s' suite.\n", name.c_str());
		SetTestingOutputColor(kTestingOutputColorDefault);
		suiteFailed = true;
#endif
	}

	// tests
	if (!suiteFailed) {
		for (auto it = tests.begin(); it != tests.end(); ++it) {
			try { // try each test
				printf("Testing %s...\n", it->first.c_str());
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
				SetTestingOutputColor(kTestingOutputColorRed);
				printf("Unhandled exception thrown during '%s' test. Aborting suite.\n", it->first.c_str());
				SetTestingOutputColor(kTestingOutputColorDefault);
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
			printf("Tearing down...\n");
			teardown(&r);
		}
		if (!r.failedChecks) {
			SetTestingOutputColor(kTestingOutputColorGreen);
			printf("All %u checks passed!\n", r.totalChecks);
			SetTestingOutputColor(kTestingOutputColorDefault);
		} else {
			SetTestingOutputColor(kTestingOutputColorRed);
			printf("%u of %u checks failed!\n", r.failedChecks, r.totalChecks);
			SetTestingOutputColor(kTestingOutputColorDefault);
		}
#ifndef DEBUG_EXCEPTIONS
	} catch (...) {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Unhandled exception thrown during teardown for '%s' suite. Aborting ALL suites.\n", name.c_str());
		SetTestingOutputColor(kTestingOutputColorDefault);
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
	SetTestingOutputColor(kTestingOutputColorDefault);

	std::map<std::string, TestResults> results;
	TestResults overall;

	size_t completedSuites = 0;

	if (suite) {
		auto it = GetTestSuiteMap()->find(suite);
		if (it == GetTestSuiteMap()->end()) {
			SetTestingOutputColor(kTestingOutputColorRed);
			printf("Test suite \"%s\" not found.\n", suite);
			SetTestingOutputColor(kTestingOutputColorDefault);
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
		for (auto it = GetTestSuiteMap()->begin(); it != GetTestSuiteMap()->end(); ++it) {
			TestResults r;
			try {
				if (it->second.run(&r)) {
					++completedSuites;
				}
			} catch (TestException) {
				break;
			}
			results[it->first] = r;
			overall += r;
		}
	}

	size_t incompletedSuites = (suite ? 1 : GetTestSuiteMap()->size()) - completedSuites;

	printf("========================= SUMMARY =========================\n");
	if (incompletedSuites) {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Warning: One or more suites were not completed. Results may be incomplete.\n");
		SetTestingOutputColor(kTestingOutputColorDefault);
	}
	printf("Total checks performed: %d\n", overall.totalChecks);
	if (!overall.failedChecks) {
		SetTestingOutputColor(kTestingOutputColorGreen);
		printf("Total checks failed: %d\n", overall.failedChecks);
		SetTestingOutputColor(kTestingOutputColorDefault);
	} else {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Total checks failed: %d\n", overall.failedChecks);
		SetTestingOutputColor(kTestingOutputColorDefault);
	}
	if (overall.abortedTests) {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Aborted tests: %d\n", overall.abortedTests);
		SetTestingOutputColor(kTestingOutputColorDefault);
	}
	if (overall.abortedSuites) {
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("Aborted suites: %d\n", overall.abortedSuites);
		SetTestingOutputColor(kTestingOutputColorDefault);
	}
	for (auto it = results.begin(); it != results.end(); ++it) {
		if (it->second.failedChecks == 0) {
			continue;
		}
		printf("-----------------------------------------------------------\n");
		SetTestingOutputColor(kTestingOutputColorRed);
		printf("%s: %d %s\n", it->first.c_str(), it->second.failedChecks, it->second.failedChecks == 1 ? "failure" : "failures");
		for (const CheckFailureMessage& m : it->second.failureMessages) {
			if (m.extra) {
				printf(m.format, m.line, m.extra);
			} else {
				printf(m.format, m.line);
			}
		}
		SetTestingOutputColor(kTestingOutputColorDefault);
	}
	printf("===========================================================\n");

	return incompletedSuites + overall.failedChecks + overall.abortedTests + overall.abortedSuites;
}

const char* TestSuiteNamer::Name;

TestSuiteMap* GetTestSuiteMap() {
	static TestSuiteMap* suites = new TestSuiteMap();
	return suites;
}

} // namespace TestDetail

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