#include <tinytest/tinytest.h>

TEST_SUITE("Math Tests") {
	TEST("1 + 1 = 2") {
		CHECK(1 + 1 == 2);
	};

	TEST("1 - 1 = 0") {
		CHECK(1 - 1 == 0);
	};
};