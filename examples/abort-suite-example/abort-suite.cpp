#include <tinytest/tinytest.h>

TEST_SUITE("Abort Suite") {

	SETUP() {
		ABORT_SUITE("Aborting suite with message test");
	};

};