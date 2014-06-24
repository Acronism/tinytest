#include <tinytest/tinytest.h>

TEST_SUITE("setup/teardown") {
	static int i = 0;

	SETUP() {
		i = 10;
	};

	TEST("") {
		CHECK(i == 10);
	};

	TEARDOWN() {
		i = 0;
	};
};