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
	FileNameOrder
};

int main(int argc, char** argv) {
	//system("pause");
	if (argc != 3)					{ return ArgumentCountError; }
	const auto& regex	= argv[RegexOrder];
	const auto& filename= argv[FileNameOrder];
	if ( !regex || !regex[0])		{ return ArgumentRegexEmptyError; }
	if ( !filename || !filename[0])	{ return ArgumentFileNameEmptyError; }

	return lr::test(filename, regex);
}