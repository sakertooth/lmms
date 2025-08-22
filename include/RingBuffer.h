/*
 * RingBuffer.h - an effective and flexible implementation of a ringbuffer for LMMS
 *
 * Copyright (c) 2014 Vesa Kivimäki
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_RING_BUFFER_H
#define LMMS_RING_BUFFER_H

#include <cstddef>
#include <vector>

#include "AudioEngine.h"

namespace lmms {

/**
 * @brief A class that represents a ring buffer. Can be used in mutlithreaded, single-producer single-consumer (SPSC)
 * scenarios. Single threaded scenarios are also fine. Users can push/pull single values or arrays of values, as
 * well as make reservations for contiguous regions within the buffer for reading or writing directly into it.
 *
 * @tparam T
 */
template <typename T> class RingBuffer
{
public:
	//! @enum ReservationType specifies the kind of reservation (i.e., either a reservation for reading or writing)
	enum class ReservationType
	{
		Read,
		Write
	};

	/**
	 * @brief A resservation for a contiguous region within the ring buffer. Reservations are returned from calls to
	 * @ref reserve, and act as RAII objects by committing data to the ring buffer on destruction of the object. Manual
	 * commits can still be made if desired.
	 *
	 * @tparam type
	 */
	template <ReservationType type> class Reservation
	{
	public:
		using Region = std::span<std::conditional_t<type == ReservationType::Write, T, const T>>;

		Reservation(const Reservation&) = delete;
		Reservation(Reservation&&) = delete;
		Reservation& operator=(const Reservation&) = delete;
		Reservation& operator=(Reservation&&) = delete;

		/**
		 * @brief Construct a new Reservation object
		 *
		 * @param buffer - a reference to the ring buffer
		 * @param region - the region associated with this reservation
		 */
		Reservation(RingBuffer<T>* buffer, Region region)
			: m_buffer(buffer)
			, m_region(region)
		{
		}

		/**
		 * @brief Destroy the Reservation object
		 * If no commits have been made to this reservation, the full region is committed.
		 */
		~Reservation()
		{
			if (!committed) { commit(m_region.size()); }
		}

		//! @returns the region associated with this reservation
		auto region() const -> Region { return m_region; }

		//! Commit @p count elements from this reservation by advancing either the read or write pointers within the
		//! ring buffer. Multiple commits can occur if the commits that happened previously did not use the entire
		//! reservation space.
		void commit(std::size_t count)
		{
			assert(region.size() >= count);
			if constexpr (type == ReservationType::Read) { m_buffer->commitReader(count); }
			else if constexpr (type == ReservationType::Write) { m_buffer->commitWriter(count); }

			m_region = m_region.subspan(count, m_region.size() - count);
			committed = true;
		}

	private:
		RingBuffer<T>* m_buffer;
		Region m_region;
		bool committed = false;
	};

	RingBuffer(std::size_t size = DEFAULT_BUFFER_SIZE)
		: m_buffer(size)
	{
	}

	/**
	 * @brief Attempts to reserve a contiguous region within the ring buffer, either for reading elements from or
	 * writing elements to.
	 *
	 * @tparam type - the reservation type
	 */
	template <ReservationType type> auto reserve() -> Reservation<type>
	{
		if constexpr (type == ReservationType::Read)
		{
			const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
			const auto readIndex = m_readIndex.load(std::memory_order_acquire);
			const auto available = readIndex < writeIndex ? writeIndex - readIndex : m_buffer.size() - readIndex;
			return {this, {&m_buffer[readIndex], available}};
		}
		else if constexpr (type == ReservationType::Write)
		{
			const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
			const auto readIndex = m_readIndex.load(std::memory_order_acquire);
			const auto available = writeIndex < readIndex ? readIndex - writeIndex - 1
														  : m_buffer.size() - writeIndex - (readIndex == 0 ? 1 : 0);
			return {this, {&m_buffer[writeIndex], available}};
		}
	}

	/**
	 * @brief Push elements from @p src into the ring buffer.
	 *
	 * @param src
	 * @return std::size_t
	 */
	[[nodiscard]] auto push(std::span<const T> src) -> std::size_t
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		auto writeIndex = m_writeIndex.load(std::memory_order_acquire);

		auto totalPushed = std::size_t{0};
		while ((writeIndex + 1) % m_buffer.size() != readIndex && totalPushed < src.size())
		{
			m_buffer[writeIndex] = src[totalPushed];
			writeIndex = (writeIndex + 1) % m_buffer.size();
			++totalPushed;
		}

		return totalPushed;
	}

	/**
	 * @brief Pull elements from the ring buffer into @p dst.
	 *
	 * @param dst
	 * @return std::size_t - the number of elements pulled
	 */
	[[nodiscard]] auto pull(std::span<T> dst) -> std::size_t
	{
		auto readIndex = m_readIndex.load(std::memory_order_acquire);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);

		auto totalPulled = std::size_t{0};
		while (readIndex != writeIndex && totalPulled < dst.size())
		{
			dst[totalPulled] = std::move(m_buffer[readIndex]);
			readIndex = (readIndex + 1) % m_buffer.size();
			++totalPulled;
		}

		m_readIndex.store(readIndex, std::memory_order_release);
		return totalPulled;
	}

	/**
	 * @brief Push @p value into the ring buffer.
	 *
	 * @param value
	 * @return true - @p value was successfully pushed
	 * @return false - the ring buffer was full and @p value could not be pushed
	 */
	[[nodiscard]] auto push(T value) -> bool
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
		if ((writeIndex + 1) == readIndex) { return false; }

		m_buffer[writeIndex] = std::move(value);
		m_writeIndex.store((writeIndex + 1) % m_buffer.size(), std::memory_order_release);
	}

	/**
	 * @brief Pull a value from the ring buffer into @p value
	 *
	 * @param value
	 * @return true - if a value was successfully pulled and moved into @p value
	 * @return false - if the ring buffer was empty and no values could be pulled
	 */
	[[nodiscard]] auto pull(T& value) -> bool
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
		if (readIndex == writeIndex) { return false; }

		value = std::move(m_buffer[readIndex]);
		m_readIndex.store((readIndex + 1) % m_buffer.size(), std::memory_order_release);
		return true;
	}

private:
	void commitWriter(std::size_t count)
	{
		const auto writeIndex = m_writeIndex.load(std::memory_order_acquire);
		m_writeIndex.store((writeIndex + count) % m_buffer.size(), std::memory_order_release);
	}

	void commitReader(std::size_t count)
	{
		const auto readIndex = m_readIndex.load(std::memory_order_acquire);
		m_readIndex.fetch_add((readIndex + count) % m_buffer.size(), std::memory_order_release);
	}

	std::vector<T> m_buffer;
	alignas(64) std::atomic<std::size_t> m_readIndex = 0;
	alignas(64) std::atomic<std::size_t> m_writeIndex = 0;
};

} // namespace lmms

#endif // LMMS_RING_BUFFER_H
