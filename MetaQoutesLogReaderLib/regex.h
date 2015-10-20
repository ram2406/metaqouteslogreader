#pragma once
namespace rx {

	inline int matchhere(const char*, const char*);
	inline int matchstar(int, const char*, const char*);


	inline
	int match(const char* regexp, const char* text) {
		if (regexp[0] == '^') {
			return matchhere(regexp + 1, text);
		}
		do {
			if (matchhere(regexp, text)) {
				return 1;
			}
		} while (*text++ != '\0');
		return 0;
	}


	inline
	int matchhere(const char* regexp, const char* text) {
		if (regexp[0] == '\0') {
			return 1;
		}
		if (regexp[1] == '*') {
			return matchstar(regexp[0], regexp + 2, text);
		}
		if (regexp[1] == '?') {
			int offset = (regexp[0] == text[0] ? +1 : +0);
			return matchhere(regexp + 2, text +offset);
		}
		if (regexp[0] == '$' && regexp[1] == '\0') {
			return *text == '\0';
		}
		if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text)) {
			return matchhere(regexp + 1, text + 1);
		}
		return 0;
	}

	inline
	int matchstar(int c, const char* regexp, const char* text) {
		do {
			if (matchhere(regexp, text)) {
				return 1;
			}
		} while (*text != '\0' && (*text++ == c || c == '.'));
		return 0;
	}
}