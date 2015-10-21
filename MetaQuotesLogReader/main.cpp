#include <stdio.h>
#include <stdlib.h>
#include <log_reader.h>

enum ErrorCode {
	ArgumentCountError = 1,
	ArgumentRegexEmptyError ,
	ArgumentFileNameEmptyError,

	UnknownError = 99
};

enum ArgumentOrder {
	ProcessNameOrder = 0,
	RegexOrder,
	FileNameOrder,
	PrintResultFlag
};

int main(int argc, char** argv) {
	
	if (argc < 3) { 
		printf(	"Uncorrect parameters!\n"
				"For example: %s \"regex\" \"logfile.txt\" [--print]\n"
				"\n Option:\n\t --print - print matched strings"
				, argv[0]);
		return ArgumentCountError; 
	}

	const auto& regex	= argv[RegexOrder];
	const auto& filename= argv[FileNameOrder];
	if ( !regex || !regex[0])		{ return ArgumentRegexEmptyError; }
	if ( !filename || !filename[0])	{ return ArgumentFileNameEmptyError; }
	bool printResult = false;
	if (argc > 3) { 
		const auto& flag = argv[PrintResultFlag];
		printResult = strcmp(flag, "--print") == 0;
	}
	system("pause");
	const auto& err_code = lr::test(filename, regex, printResult);
	return  err_code 
			? err_code + UnknownError +1 
			: 0;
}