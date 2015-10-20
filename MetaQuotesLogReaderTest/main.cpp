#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <chrono>
#include <thread_safe_queue.h>


static struct hack {
	~hack() { system("pause"); }	//wait user to press any key
} h;
namespace /* simple exec test */{
	TEST_CASE("test of exec program with valid parameters") {
		CHECK(system(R"(del logfilename.txt && del logfilename1.txt)") == 0);

		CHECK(system(R"(echo.  2> logfilename.txt)") == 0);
		CHECK(system(R"(logreader "regex" "logfilename.txt" )") == 105);
		
		CHECK(system(R"(logreader "regex" "logfilename.txt1" )") == 102);
		CHECK(system(R"(echo regex text \n rege text > logfilename1.txt)") == 0);
		
		CHECK(system(R"(logreader "^regex" "logfilename1.txt" )") == 0);
		
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

namespace /* main tests */ {
	TEST_CASE("main-test") {
		
		auto t1 = std::chrono::high_resolution_clock::now();
		CHECK(system(R"(logreader "^[2015-10-11 16:.*WARN.*Not Found?$" "../MetaQuotesLogReaderTest/teamcity-vcs.log.2" --print)") == 0);
		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "\n Time: " << duration.count() * 0.001 << "s" << std::endl;
		
		
	}
}