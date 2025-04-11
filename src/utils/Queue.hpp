// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

// this should be documented
#include <atomic>
#include <utility>
extern "C" {
#include "pico/lock_core.h"
}

#include <array>
#include <cstdint>
#include <type_traits>

#include "RingBuffer.hpp"

template <typename T, size_t N>
class BlockingQueue : protected RingBuffer<T, N> {
public:
	BlockingQueue() {
		lock_init(&d_core, next_striped_spin_lock_num());
	}

	inline size_t Size() const {
		auto   save = lock();
		size_t size = this->d_count;
		unlock(save);
		return size;
	}

	inline bool Empty() const {
		return Size() == 0;
	}

	inline bool Full() const {
		return Size() == N;
	}

	template <typename U> inline bool TryAdd(U &&obj) {
		return add(std::forward<U>(obj), false);
	}

	inline bool TryRemove(T &obj) {
		return remove(obj, false);
	}

	template <typename... Args> inline bool TryEmplace(Args &&...args) {
		return BlockingQueue::emplace(false, std::forward<Args>(args)...);
	}

	template <typename U> inline void AddBlocking(U &&obj) {
		this->add(std::forward<U>(obj), true);
	}

	inline void RemoveBlocking(T &obj) {
		this->remove(obj, true);
	}

	template <typename... Args> inline void EmplaceBlocking(Args &&...args) {
		BlockingQueue::emplace(true, std::forward<Args>(args)...);
	}

protected:
	lock_core_t d_core;

	inline uint32_t lock() const {
		return spin_lock_blocking(d_core.spin_lock);
	}

	inline void unlock(uint32_t saved) const {
		spin_unlock(d_core.spin_lock, saved);
	}

	inline void unlock_wait(uint32_t saved) const {
		lock_internal_spin_unlock_with_wait(&d_core, saved);
	}

	inline void unlock_notify(uint32_t saved) const {
		lock_internal_spin_unlock_with_notify(&d_core, saved);
	}

	template <typename U> inline bool add(U &&obj, bool block) {
		do {
			auto save = lock();
			if (this->d_count != N) {
				this->insert(std::forward<U>(obj));
				unlock_notify(save);
				return true;
			}
			if (block) {
				unlock_wait(save);
			} else {
				unlock(save);
				return false;
			}
		} while (true);
	}

	inline bool remove(T &obj, bool block) {
		do {
			auto save = lock();
			if (this->d_count > 0) {
				this->pop(obj);
				unlock_notify(save);
				return true;
			}
			if (block) {
				unlock_wait(save);
			} else {
				unlock(save);
				return false;
			}
		} while (true);
	}

	template <typename... Args>
	inline bool emplace(bool block, Args &&...args) {
		do {
			auto save = lock();
			if (this->d_count != N) {
				RingBuffer<T, N>::emplace(std::forward<Args>(args)...);
				unlock_notify(save);
				return true;
			}
			if (block) {
				unlock_wait(save);
			} else {
				unlock(save);
				return false;
			}
		} while (true);
	}

	inline void increment(uint16_t &value) {
		if (++value >= N) {
			value = 0;
		}
	}
};

template <typename T, uint16_t N> class ConcurentQueue {
public:
	inline size_t Size() const {
		auto tail = d_consumerHead.load(std::memory_order_relaxed);
		auto head = d_producerTail.load(std::memory_order_relaxed);

		if (tail > head) {
			return head - tail + N;
		}
		return head - tail;
	}

	inline bool Empty() const {
		return d_consumerHead.load(std::memory_order_relaxed) ==
		       d_producerTail.load(std::memory_order_relaxed);
	}

	inline bool Full() const {
		return Size() == Capacity;
	}

	template <typename U> inline bool TryAdd(U &&obj) {

		auto oldProducerHead = d_producerHead.load(std::memory_order_relaxed);
		do {

			auto consumerTail = d_consumerTail.load(std::memory_order_acquire);

			if (oldProducerHead + 1 == consumerTail ||
			    (oldProducerHead == Capacity && consumerTail == 0)) {
				// we are full.
				return false;
			}

		} while (d_producerHead.compare_exchange_weak(
		             oldProducerHead,
		             oldProducerHead + 1,
		             std::memory_order_relaxed,
		             std::memory_order_relaxed
		         ) == false);

		d_objects[oldProducerHead] = std::forward<U>(obj);
		while (d_producerTail.load(std::memory_order_relaxed) != oldProducerHead
		) {
			tight_loop_contents();
		}
		d_producerTail.store(oldProducerHead + 1, std::memory_order_release);
		return true;
	}

	template <typename... Args> inline bool TryEmplace(Args &&...args) {
		auto oldProducerHead = d_producerHead.load(std::memory_order_relaxed);
		do {

			auto consumerTail = d_consumerTail.load(std::memory_order_acquire);

			if (oldProducerHead + 1 == consumerTail ||
			    (oldProducerHead == Capacity && consumerTail == 0)) {
				// we are full.
				return false;
			}

		} while (d_producerHead.compare_exchange_weak(
		             oldProducerHead,
		             oldProducerHead + 1,
		             std::memory_order_relaxed,
		             std::memory_order_relaxed
		         ) == false);

		d_objects[oldProducerHead] = T(std::forward<Args>(args)...);
		while (d_producerTail.load(std::memory_order_relaxed) != oldProducerHead
		) {
			tight_loop_contents();
		}
		d_producerTail.store(oldProducerHead + 1, std::memory_order_release);
		return true;
	}

	inline bool TryRemove(T &obj) {
		auto oldConsumerHead = d_consumerHead.load(std::memory_order_relaxed);
		do {

			auto producerTail = d_producerTail.load(std::memory_order_acquire);
			if (producerTail == oldConsumerHead) {
				return false;
			}

		} while (d_consumerHead.compare_exchange_weak(
		             oldConsumerHead,
		             oldConsumerHead + 1,
		             std::memory_order_relaxed,
		             std::memory_order_relaxed
		         ) == false);

		obj = d_objects[oldConsumerHead];

		while (d_consumerTail.load(std::memory_order_relaxed) != oldConsumerHead
		) {
			tight_loop_contents();
		}

		d_consumerTail.store(oldConsumerHead + 1, std::memory_order_release);
		return true;
	}

private:
	constexpr static size_t      Capacity = N - 1;
	std::array<T, N>             d_objects;
	volatile std::atomic<uint16_t> d_producerHead, d_producerTail,
	    d_consumerHead, d_consumerTail;
};
