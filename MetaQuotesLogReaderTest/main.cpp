#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <chrono>
#include <thread_safe_queue.h>


static struct hack {
	~hack() { system("pause"); }	//wait user to press any key
} h;



namespace /*read file*/ {
	TEST_CASE("test of read file") {
		char string_buf[2048] = { 0 };
		char string_buf2[sizeof(string_buf)] = { 0 };
		spec::CFile file;
		CHECK(file.OpenForRead("../MetaQuotesLogReaderTest/teamcity-vcs.log.2"));
		CHECK(file.ReadString(string_buf, sizeof(string_buf)));
		unsigned long pos = file.GetPosition();
		CHECK(file.ReadString(string_buf, sizeof(string_buf)));
		file.SetPosition(pos);
		CHECK(file.ReadString(string_buf2, sizeof(string_buf2)));
		CHECK(strcmp(string_buf, string_buf2) == 0);

		unsigned long pos0 = file.GetPosition();
		CHECK(file.ReadString(string_buf2, sizeof(string_buf2)));
		file.SetPosition(pos0);
		unsigned long pos1, npos1;
		file.ReadStrings(string_buf, sizeof(string_buf), pos1, npos1);
		CHECK(pos0 == pos1);

		file.SetPosition(pos1);
		unsigned long pos2, npos2;
		file.ReadStrings(string_buf, sizeof(string_buf), pos2, npos2);
		CHECK(pos1 == pos2);
		
		CHECK(file.Close());
	}
}


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
		CHECK(system(R"(logreader "2015-10-13.*WARN.*Not Found?$" "../MetaQuotesLogReaderTest/teamcity-vcs.log.2")") == 0);
		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "\n Time: " << duration.count() * 0.001 << "s" << std::endl;
	}

	TEST_CASE("main-test #2") {

		auto t1 = std::chrono::high_resolution_clock::now();
		CHECK(system(R"(logreader "^[2015-10-11 16:.*WARN.*Not Found?$" "../MetaQuotesLogReaderTest/teamcity-vcs.log.2" --print)") == 0);

		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "\n Time: " << duration.count() * 0.001 << "s" << std::endl;


		
	}

	TEST_CASE("main-test #3") {

		auto t1 = std::chrono::high_resolution_clock::now();
		CHECK(system(R"(logreader "WARN.*lerg.*Not Found?$" "../MetaQuotesLogReaderTest/teamcity-vcs.log.2")") == 0);

		auto t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		std::cout << "\n Time: " << duration.count() * 0.001 << "s" << std::endl;



	}
}