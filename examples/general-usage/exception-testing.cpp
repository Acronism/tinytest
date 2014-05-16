#include <tinytest/tinytest.h>

#include <vector>

TEST_SUITE("Container Tests") {

	TEST("bounds testing") {
		std::vector<int> v = {0, 1, 2};

		for (size_t i = 0; i < 3; ++i) {
			CHECK(v.at(i) == i);
		}
		
		CHECK_EXCEPTION(v.at(4), std::exception);
		CHECK_NO_EXCEPTION(v.at(0));
	};

};