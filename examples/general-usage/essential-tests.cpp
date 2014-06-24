#include <tinytest/tinytest.h>

TEST_SUITE("Essential") {
	static int i = 0;

	SETUP() {
		i = 10;
		CHECK_ESSENTIAL(i == 10); // if this fails, the suite will be aborted
	};

	TEARDOWN() {
		CHECK_ESSENTIAL(i == 10);
		i = 0;
	};
};