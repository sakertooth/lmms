/*
 * AudioBuffer.h
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_BUFFER_H
#define LMMS_AUDIO_BUFFER_H

#include <QString>
#include <vector>

#include "LmmsTypes.h"
#include "lmms_export.h"

namespace lmms {
/**
 * @class AudioBuffer
 * @brief Represents an in-memory audio buffer containing interleaved floating-point samples.
 */
class LMMS_EXPORT AudioBuffer
{
public:
	//! Creates a silent, empty audio buffer.
	AudioBuffer() = default;

	//! Create a silent audio buffer containing the number of frames and channels specified with the given sample rate.
	AudioBuffer(f_cnt_t numFrames, f_cnt_t numChannels, sample_rate_t sampleRate);

	//! @returns a reference to the sample at frame @a frameIndex and channel @a channelIndex.
	auto sampleAt(f_cnt_t frameIndex, ch_cnt_t channelIndex) -> sample_t&;

	//! Converts the audio buffer into a Base64 string representation.
	//! Needed for seralization purposes.
	auto toBase64() const -> QString;

	//! @returns a pointer to the raw interleaved data in the audio buffer.
	//! Needed when working with C APIs.
	auto data() -> float* { return m_data.data(); }

	//! @returns the sample rate associated with this audio buffer.
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	//! @returns the number of audio frames in the audio buffer.
	auto numFrames() const -> f_cnt_t { return m_numFrames; }

	//! @returns the number of channels in the audio buffer.
	auto numChannels() const -> ch_cnt_t { return m_numChannels; }

	//! Creates an audio buffer from the given audio file specified by @a path on disk.
	static AudioBuffer createFromFile(const QString& path);

private:
	std::vector<float> m_data;
	sample_rate_t m_sampleRate = 0;
	ch_cnt_t m_numChannels = 0;
	f_cnt_t m_numFrames = 0;
};

} // namespace lmms

#endif // LMMS_AUDIO_BUFFER_H
