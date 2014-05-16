#pragma once

#include <stdio.h>

#include <string>
#include <map>
#include <functional>
#include <list>

// CRAZY BEHIND-THE-SCENES SETUP STUFF

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

void SetTestingOutputColor(TestingOutputColor color);

// FUNCTIONS USED TO RUN TESTS

int RunTests(const char* suite = nullptr);

// MACROS USED TO CREATE TESTS

#define CONCATENATE_INNER(a, b) a ## b
#define CONCATENATE(a, b) CONCATENATE_INNER(a, b)

#define TEST_SUITE(name) \
	static TestSuiteNamer CONCATENATE(gTestSuiteNamer, __COUNTER__) = name; \
	static TestSuiteCreator CONCATENATE(gTestSuiteCreator, __COUNTER__) = [](TestSuite* suite)

#define SETUP() \
	suite->setup = [](TestResults* results)

#define TEARDOWN() \
	suite->teardown = [](TestResults* results)

#define TEST(name) \
	suite->tests[name] = [](TestResults* results)

#define ABORT_SUITE(...) \
	const char* format = "Abort Suite called in " __FILE__ " on line %d:\n    "; \
	SetTestingOutputColor(kTestingOutputColorRed); \
	printf(format, __LINE__); \
	printf(__VA_ARGS__); \
	printf("\n"); \
	SetTestingOutputColor(kTestingOutputColorDefault); \
	results->failureMessages.push_back(CheckFailureMessage(format, __LINE__)); \
	throw TestExceptionAbortSuite;

#define ABORT_TEST() \
	const char* format = "Abort Test called in " __FILE__ " on line %d!\n"; \
	SetTestingOutputColor(kTestingOutputColorRed); \
	printf(format, __LINE__); \
	SetTestingOutputColor(kTestingOutputColorDefault); \
	results->failureMessages.push_back(CheckFailureMessage(format, __LINE__)); \
	throw TestExceptionAbortTest;

// exception != failed check
#define CHECK(condition) \
	++results->totalChecks; \
	if (!(condition)) { \
		++results->failedChecks; \
		const char* format = "Failed check in " __FILE__ " on line %d!\n"; \
		SetTestingOutputColor(kTestingOutputColorRed); \
		printf(format, __LINE__); \
		SetTestingOutputColor(kTestingOutputColorDefault); \
		results->failureMessages.push_back(CheckFailureMessage(format, __LINE__)); \
	}

// exception != failed check
#define CHECK_ESSENTIAL(condition) \
	++results->totalChecks; \
	if (!(condition)) { \
		++results->failedChecks; \
		const char* format = "Failed essential check in " __FILE__ " on line %d! Aborting test.\n"; \
		SetTestingOutputColor(kTestingOutputColorRed); \
		printf(format, __LINE__); \
		SetTestingOutputColor(kTestingOutputColorDefault); \
		results->failureMessages.push_back(CheckFailureMessage(format, __LINE__)); \
		throw TestExceptionAbortTest; \
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

