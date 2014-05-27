#include <tinytest/tinytest.h>

#include <thread>

TEST_SUITE(__FILE__) {
	TEST(__FILE__ ", Test 1") {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		CHECK(true);
	};
	TEST(__FILE__ ", Test 2") {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		CHECK(true);
	};
	TEST(__FILE__ ", Test 3") {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		CHECK(true);
	};
	TEST(__FILE__ ", Test 4") {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		CHECK(true);
	};
	TEST(__FILE__ ", Test 5") {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		CHECK(true);
	};
};