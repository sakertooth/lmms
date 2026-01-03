/*
 * SampleBuffer.h - container-class SampleBuffer
 *
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

#ifndef LMMS_SAMPLE_BUFFER_H
#define LMMS_SAMPLE_BUFFER_H

#include <QString>
#include <memory>
#include <optional>

#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
/**
 * @brief Immutable container for audio sample data.
 *
 * SampleBuffer represents a contiguous block of audio data meant for holding loaded sample files, samples from Base64,
 * as well as samples from raw memory (if needed). It shares ownership with the audio data, meaning that copies of the
 * same SampleBuffer object do not copy the underlying audio data. The audio data is shared, hence why SampleBuffer is
 * immutable.
 *
 * @todo Base64 serialization of audio is discouraged, as it can be too big to store in the project file. This should be
 * removed in the future.
 */
class LMMS_EXPORT SampleBuffer
{
public:
	/**
	 * @brief Construct an empty SampleBuffer.
	 *
	 * The resulting buffer contains no frames and has a sample rate of 0, but points to a valid buffer.
	 */
	SampleBuffer() = default;

	/**
	 * @brief Construct a SampleBuffer from existing sample data.
	 * 
	 * @param data 			Unique ownership of an array of SampleFrame's
	 * @param numFrames 	Number of audio frames in the buffer.
	 * @param sampleRate 	The sample rate associated with the buffer
	 * @param path 			Optional file path (used for identity)
	 */
	SampleBuffer(std::unique_ptr<SampleFrame[]> data, f_cnt_t numFrames, sample_rate_t sampleRate,
		const QString& path = QString{});

	/**
	 * @brief Serializes the buffer contents to a Base64-encoded string.
	 *
	 * The encoded data contains the raw audio data and can later be reconstructed using fromBase64().
	 *
	 * @return Base64 representation of the sample data.
	 */
	auto toBase64() const -> QString;

	/**
	 * @brief Checks whether the buffer contains any audio data.
	 * 
	 * @return true if the buffer is empty, false otherwise.
	 */
	auto empty() const -> bool { return m_numFrames == 0; }

	/**
	 * @brief Returns a pointer to the audio data.
	 *
	 * The pointer returned is always valid and never null.
	 * 
	 * @return const SampleFrame* 
	 */
	auto data() const -> const SampleFrame* { return m_data.get(); }

	//! @returns the sample rate of the buffer.
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	//! @returns the number of audio frames within the buffer.
	auto numFrames() const -> f_cnt_t { return m_numFrames; }

	/**
	 * @brief Returns the source path associated with this buffer.
	 *
	 * This is typically the file path the samples were loaded from, but
	 * may be empty if the buffer was constructed from memory or Base64 data.
	 *
	 * @return const QString&
	 */
	auto path() const -> const QString& { return m_path; }

		/**
	 * @brief Loads a SampleBuffer from an audio file.
	 *
	 * @param path Path to the audio file.
	 * @return A SampleBuffer on success, or std::nullopt on failure.
	 */
	static auto fromFile(const QString& path) -> std::optional<SampleBuffer>;

	/**
	 * @brief Constructs a SampleBuffer from a Base64-encoded string.
	 *
	 * @param str         Base64-encoded audio data.
	 * @param sampleRate  Sample rate of the audio data.
	 * @return A SampleBuffer on success, or std::nullopt on failure.
	 */
	static auto fromBase64(const QString& str, sample_rate_t sampleRate) -> std::optional<SampleBuffer>;

private:
	inline static const auto emptyBuffer = std::make_shared<SampleFrame[]>(0);
	std::shared_ptr<const SampleFrame[]> m_data = emptyBuffer;
	f_cnt_t m_numFrames = 0;
	sample_rate_t m_sampleRate = 0;
	QString m_path;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
