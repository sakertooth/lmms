/*
 * SpscLockfreeQueue.h
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_SPSC_LOCKFREE_QUEUE_H
#define LMMS_SPSC_LOCKFREE_QUEUE_H

#include <atomic>
#include <optional>
#include <vector>

namespace lmms {
//! A single producer, single consumer lockfree queue.
//! There can only be one reader and one writer at any given point in time
//! when this queue is used.
template <typename T> class SpscLockfreeQueue
{
public:
	//! Creates the queue with the given maximum `capacity`.
	SpscLockfreeQueue(std::size_t capacity)
		: m_queue(capacity)
	{
	}

	SpscLockfreeQueue(SpscLockfreeQueue&& other) noexcept
		: m_queue(std::move(other.m_queue))
	{
	}

	SpscLockfreeQueue(const SpscLockfreeQueue&) = delete;
	SpscLockfreeQueue& operator=(const SpscLockfreeQueue&) = delete;
	SpscLockfreeQueue& operator=(SpscLockfreeQueue&&) = delete;
	~SpscLockfreeQueue() = default;

	//! Push `value` into the queue.
	//! Blocks if no space is available.
	void push(T value)
	{
		m_full.wait(true);

		const auto writeIndex = m_writeIndex.load();
		const auto nextIndex = (writeIndex + 1) % m_queue.size();

		m_queue[writeIndex] = std::move(value);
		m_writeIndex.store(nextIndex, std::memory_order_release);

		if (nextIndex == m_readIndex.load(std::memory_order_acquire)) { m_full.test_and_set(); }
		m_empty.test_and_set();
	}

	//! Tries to push `value` into the queue.
	//! Returns `false` if there is no space available and `true` otherwise.
	bool tryPush(T value);

	//! Pop `value` from the queue and returns it.
	//! Blocks if there is no `T` within the queue that can be read.
	T pop()
	{
		m_empty.wait(true);

		const auto readIndex = m_readIndex.load();
		const auto nextIndex = (readIndex + 1) % m_queue.size();

		const auto value = std::move(m_queue[readIndex]);
		m_readIndex.store(nextIndex, std::memory_order_release);

		if (nextIndex == m_writeIndex.load(std::memory_order_acquire)) { m_empty.test_and_set(); }
		m_full.test_and_set();

		return value;
	}

	//! Tries to pop `value` from the queue.
	//! Returns `std::nullopt` if there is no available value and `true` otherwise.
	std::optional<T> tryPop();

private:
	std::vector<T> m_queue;
	std::atomic<std::size_t> m_writeIndex = 0;
	std::atomic<std::size_t> m_readIndex = 0;
	std::atomic_flag m_full = ATOMIC_FLAG_INIT;
	std::atomic_flag m_empty = ATOMIC_FLAG_INIT;
};
} // namespace lmms

#endif // LMMS_SPSC_LOCKFREE_QUEUE_H