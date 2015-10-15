#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <log_reader.h>

static struct hack {
	~hack() { system("pause"); }
} h;
namespace /* simple exec test */{
	TEST_CASE("test of exec program with valid parameters") {
		CHECK(system(R"(del logfilename.txt && del logfilename1.txt)") == 0);

		CHECK(system(R"(echo.  2> logfilename.txt)") == 0);
		CHECK(system(R"(logreader "regex" "logfilename.txt" )") == 104);
		
		CHECK(system(R"(logreader "regex" "logfilename.txt1" )") == 102);
		CHECK(system(R"(echo regex text > logfilename1.txt)") == 0);
		
		CHECK(system(R"(logreader "^regex*" "logfilename1.txt" )") == 0);
		
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