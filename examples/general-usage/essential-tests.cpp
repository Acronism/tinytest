#include <tinytest/tinytest.h>

static int i = 0;

TEST_SUITE("Essential") {

	SETUP() {
		i = 10;
		CHECK_ESSENTIAL(i == 10); // if this fails, the suite will be aborted
	};

	TEARDOWN() {
		CHECK_ESSENTIAL(i == 10);
		i = 0;
	};
};