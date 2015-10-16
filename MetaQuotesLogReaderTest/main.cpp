#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <thread_safe_queue.h>
#include <log_reader.h>

static struct hack {
	~hack() { system("pause"); }	//wait user to press any key
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

namespace /* conainers test */ {
	TEST_CASE("test of queue") {
		containers::CQueue<int> queue;
		queue.Push(5);
		queue.Push(0);
		queue.Push(1);
		queue.Push(10);
		queue.Push(100);

		int data1, data2;
		queue.Pop(data1);
		while (queue.Pop(data2));
		CHECK(data1 == 5);
		CHECK(data2 == 100);
	}

	TEST_CASE("test of thread safe queue") {
		containers::CThreadSafeQueue<int> queue;
		queue.Push(5);
		queue.Push(0);
		queue.Push(1);
		queue.Push(10);
		queue.Push(100);

		int data1, data2;
		queue.Pop(data1);
		while (queue.Pop(data2));
		CHECK(data1 == 5);
		CHECK(data2 == 100);
	}
}