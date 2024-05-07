
#pragma once
#include <string>

static inline bool iequal(const char *src, size_t len, const char *dst) {
	if (strlen(dst) != len) {
		return false;
	}
	for (size_t i = 0; i < len; i++) {
		if (std::tolower(src[i]) != std::tolower(dst[i])) {
			return false;
		}
	}
	return true;
}

static inline std::string trimSpace(std::string str) {
	str = str.substr(str.find_first_not_of(" "));
	return str.substr(0,  str.find_last_not_of(" ") + 1);
}