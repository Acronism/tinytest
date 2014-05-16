#include <tinytest/tinytest.h>

static int i = 0;

TEST_SUITE("setup/teardown") {

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