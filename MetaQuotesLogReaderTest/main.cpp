#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

static struct hack {
	~hack() { system("pause"); }
} h;
namespace /* simple exec test */{
	TEST_CASE("test of exec program with valid parameters") {
		CHECK(system(R"(logreader "regex" "logfilename.txt" )") == 0);
	}

	TEST_CASE("test of exec program without parameters") {
		CHECK(system(R"(logreader)") == 1);
	}

	TEST_CASE("test of exec program with invalid parameters") {
		CHECK(system(R"(logreader "" "")") == 2);
	}

	TEST_CASE("test of exec program with invalid parameter #2") {
		CHECK(system(R"(logreader "1" "")") == 3);
	}

}