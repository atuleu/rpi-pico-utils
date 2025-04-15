// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <pico/platform/panic.h>
#include <type_traits>

template <typename T, uint8_t N, std::enable_if_t<N >= 1> * = nullptr>
class RingBuffer {
public:
	template <typename U> inline bool insert(U &&obj) {
		if (full()) {
			return false;
		}
		d_data[d_head] = std::forward<U>(obj);
		increment(d_head);
		return true;
	}

	template <typename... Args> inline bool emplace(Args &&...args) {
		if (full()) {
			return false;
		}

		d_data[d_head] = T{std::forward<Args>(args)...};
		increment(d_head);
		return true;
	}

	inline bool pop(T &obj) {
		if (empty()) {
			return false;
		}
		obj = std::move(d_data[d_tail]);
		increment(d_tail);
		return true;
	}

	inline uint8_t size() const {
		if (d_head >= d_tail) {

			return d_head - d_tail;
		}
		return d_head - d_tail + N;
	}

	inline bool empty() const {
		return d_head == d_tail;
	}

	inline bool full() const {
		if (d_head == N - 1) {
			return d_tail == 0;
		}
		return d_head + 1 == d_tail;
	}

protected:
	inline void increment(uint8_t &pointer) {
		pointer += 1;
		if (pointer >= N) {
			pointer = 0;
		}
	}

	std::array<T, N> d_data;
	uint8_t          d_head = 0;
	uint8_t          d_tail = 0;
};
