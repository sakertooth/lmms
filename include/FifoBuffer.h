/*
 * FifoBuffer.h - FIFO fixed-size buffer
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef LMMS_FIFO_BUFFER_H
#define LMMS_FIFO_BUFFER_H

#include <atomic>
#include <optional>
#include <vector>

namespace lmms {
//! A single producer single consumer wait-free and lock-free fixed size FIFO queue.
template <typename T> class FifoBuffer
{
public:
	FifoBuffer(size_t size)
		: m_buffer(size)
	{
	}

	//! Push `value` into the queue, busy-waiting if it is full.
	void push(T value)
	{
		const auto newWriteIndex = (m_writeIndex + 1) % m_buffer.size();
		while (newWriteIndex == m_readIndex) {}
		m_buffer[m_writeIndex] = std::move(value);
		m_writeIndex = newWriteIndex;
	}

	//! Pop and return a value out of the queue, busy-waiting if there are no items available.
	auto pop() -> T
	{
		while (m_readIndex == m_writeIndex) {}
		const auto value = m_buffer[m_readIndex];
		m_readIndex = (m_readIndex + 1) % m_buffer.size();
		return value;
	}

	//! Attempts to push `value` into the queue, returning `true` if it was pushed and `false` otherwise.
	auto tryPush(T value) -> bool
	{
		if ((m_writeIndex + 1) % m_buffer.size() == m_readIndex) { return false; }
		push(std::move(value));
		return true;
	}

	//! Attempts to pop a value out of the queue, returning it if possible and `std::nullopt` otherwise.
	auto tryPop() -> std::optional<T>
	{
		if (m_readIndex == m_writeIndex) { return std::nullopt; }
		return pop();
	}

	//! Waits until all items have been read from the queue.
	void waitForFullRead() const
	{
		while (m_readIndex != m_writeIndex) {}
	}

	//! Return the size of this FIFO queue.
	auto size() const -> size_t { return m_buffer.size(); }

	//! Returns `true` if this queue is empty and `false` otherwise.
	auto empty() const -> bool { return m_readIndex == m_writeIndex; }

private:
	std::vector<T> m_buffer;
	std::atomic<size_t> m_readIndex = 0;
	std::atomic<size_t> m_writeIndex = 0;
};
} // namespace lmms

#endif // LMMS_FIFO_BUFFER_H
