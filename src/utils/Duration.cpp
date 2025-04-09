#include "Duration.hpp"

#include <iomanip>
#include <pico/types.h>
#include <sstream>

std::string FormatDuration(int64_t duration) {
	std::ostringstream oss;

	if (std::abs(duration) < 1000) {
		oss << duration
		    << "us"; // would like to use UTF-8, but messes all the alignements
		return oss.str();
	}
	oss << std::fixed << std::setprecision(3);

	if (std::abs(duration) < 1000000) {
		oss << float(duration) / 1000.0f << "ms";

	} else {
		oss << float(duration) / 1e6 << "s";
	}
	return oss.str();
}
