// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include <cstdio>

#ifdef NDEBUG
#define debugf(...)                                                            \
	do {                                                                       \
	} while (0)
#else
#define debugf(...)                                                            \
	do {                                                                       \
		printf(__VA_ARGS__);                                                   \
	} while (0)
#endif
