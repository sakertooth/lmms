/*
 * SampleBuffer.h
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

#include <chrono>
#include <filesystem>
#include <vector>

#include "lmms_export.h"

namespace lmms {

//! A class that represents and stores a non-interleaved (planar) audio buffer.
//! The buffer stores its samples in the form of LLLRRR inside one contiguous buffer on the heap.
class LMMS_EXPORT SampleBuffer
{
public:
	SampleBuffer(const SampleBuffer&);
	SampleBuffer& operator=(const SampleBuffer&);
	SampleBuffer(SampleBuffer&&) noexcept;
	SampleBuffer& operator=(SampleBuffer&&) noexcept;

	//! Creates an empty buffer.
	SampleBuffer(int numChannels, int numFrames, int sampleRate);

	//! Creates a buffer from interleaved audio data contained within @p src.
	//! The layouts will be converted as necessary.
	SampleBuffer(const float* src, int numChannels, int numFrames, int sampleRate);

	//! Creates a buffer from planar audio data contained within @p src.
	SampleBuffer(const float** src, int numChannels, int numFrames, int sampleRate);

	//! Creates a buffer by loading data from the given audio file @p path.
	//! If the given sample rate does not match the sample rate of the audio file, the sample rate is
	//! converted.
	SampleBuffer(const std::filesystem::path& path, int sampleRate);

	//! Destroys the buffer.
	~SampleBuffer() = default;

	//! Returns the plane at the given channel index.
	float* operator[](std::size_t channel) { return m_planes[channel]; }

	//! Returns the plane at the given channel index.
	const float* operator[](std::size_t channel) const { return m_planes[channel]; }

	//! Converts the stored planar audio data into an interleaved form stored in @p dst.
	//! @p dst is expected to have the same size as the buffer.
	void convertToInterleaved(float* dst) const;

	//! Return the planes (0: LLL, 1: RRR, etc) for this buffer.
	auto data() -> float** { return m_planes.data(); }

	//! Return the planes (0: LLL, 1: RRR, etc) for this buffer.
	auto data() const -> const float* const*  { return m_planes.data(); }

	//! Calculate the duration of the buffer.
	auto duration() const -> std::chrono::milliseconds;

	//! Returns the number of the channels this buffer contains.
	auto numChannels() const -> int { return m_numChannels; }

	//! Returns the number of frames this buffer contains.
	//! A frame represents a collection of samples, one for each channel.
	auto numFrames() const -> int { return m_numFrames; }

	//! Add @p src1 and @p src2 element-wise and store the results in @p dst.
	//! The buffers can overlap.
	//! It is assumed that all buffers are of the same size.
	static void add(SampleBuffer& dst, const SampleBuffer& src1, const SampleBuffer& src2);

	//! Do an element-wise multiplication with the samples in @p dst with a buffer of elements inside @p src.
	//! Both @p dst and @p src are expected to have a size of @p size.
	static void multiply(SampleBuffer& dst, const float* src, std::size_t size);

private:
	std::vector<float> m_data;
	std::vector<float*> m_planes;
	int m_numChannels;
	int m_numFrames;
	int m_sampleRate;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
