// SPDX-License_identifier:  LGPL-3.0-or-later

#include <utils/Defer.hpp>

#include <utils/Queue.hpp>

#include <utils/FlashStorage.hpp>

int main() {
	BlockingQueue<int, 8> foo;
	foo.EmplaceBlocking(0);
	int res;
	foo.RemoveBlocking(res);
	return res;
}
