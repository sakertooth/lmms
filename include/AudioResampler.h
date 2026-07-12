/*
 * AudioResampler.h
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

#ifndef LMMS_AUDIO_RESAMPLER_H
#define LMMS_AUDIO_RESAMPLER_H

#ifdef LMMS_DEBUG
#include <iostream>
#endif

#include <memory>
#include <samplerate.h>

#include "AudioBufferView.h"
#include "lmms_export.h"

namespace lmms {

/**
 * @class AudioResampler
 * @brief A utility class for resampling interleaved audio buffers using various resampling algorithms.
 *
 * This class provides support for zero-order hold, linear, and several levels of sinc-based resampling.
 *
 * @tparam Channels The number of audio channels to resample. Defaults to 2 (stereo).
 */
template <ch_cnt_t Channels = 2> class LMMS_EXPORT AudioResampler
{
public:
	/**
	 * @struct Result
	 * @brief Result of a resampling operation.
	 */
	struct Result
	{
		f_cnt_t inputFramesUsed;	   //!< The number of input frames used during processing.
		f_cnt_t outputFramesGenerated; //!< The number of output frames generated during processing.
	};

	/**
	 * @struct StreamBuffer
	 * @brief Represents a buffer storage used when resampling from audio streams.
	 * @tparam Frames The buffer capacity measured in audio frames.
	 * @see StreamFn
	 */
	template <f_cnt_t Frames = 128> struct StreamBuffer
	{
		std::array<float, Frames * Channels> buffer;
		f_cnt_t index = 0;
		f_cnt_t count = 0;
	};

	/**
	 * @brief Constructs an `AudioResampler` instance.
	 * @param mode The resampling mode to use.
	 */
	AudioResampler(int mode)
		: m_state(src_new(mode, Channels, &m_error))
	{
		if (!m_state)
		{
			throw std::runtime_error{std::string{"Error when creating AudioResampler state: "} + src_strerror(m_error)};
		}
	}

	/**
	 * @brief Process a block of interleaved audio input from `input` and resample it into `output`.
	 *
	 * @param input The interleaved audio input.
	 * @param output The interleaved audio output.
	 *
	 * @throws `std::invalid_argument` if a channel mismatch has been detected.
	 * @throws `std::runtime_error` if the resampling process has failed.
	 *
	 * @remark This utility class does not cache the input and output buffers, making it stateless. In other words,
	 * `input` is directly resampled into the `output`.
	 *
	 * @returns the result of the resampling process. See @ref Result for more details.
	 */
	[[nodiscard]] auto process(
		InterleavedBufferView<const float, Channels> input, InterleavedBufferView<float, Channels> output) -> Result
	{
		auto data = SRC_DATA{.data_in = input.data(),
			.input_frames = input.frames(),
			.data_out = output.data(),
			.output_frames = output.frames(),
			.ratio = m_ratio,
			.end_of_input = 0};

		if ((m_error = src_process(m_state.get(), &data)))
		{
#ifdef LMMS_DEBUG
			std::cerr << "AudioResampler: " << src_strerror(m_error) << '\n';
#endif
			std::ranges::fill(output, 0.f);
			return {0, 0};
		}

		return {static_cast<f_cnt_t>(data.input_frames_used), static_cast<f_cnt_t>(data.output_frames_gen)};
	}

	/**
	 * @brief Process a block of interleaved audio input from @a streamBuffer into @a output.
	 *
	 * @tparam Capacity The capacity of @a streamBuffer.
	 * @tparam RefillFn The function used to refill @a streamBuffer.
	 * @param streamBuffer The stream buffer where incoming audio samples are stored and refilled as needed.
	 * @param output The final output destination.
	 * @return true if the resampling process was successful, false if an error occurred.
	 *
	 */
	template <f_cnt_t Capacity = 128, typename RefillFn>
	void process(RefillFn refillFn, StreamBuffer<Capacity>& streamBuffer, InterleavedBufferView<float, Channels> output)
	{
		auto outputGenerated = 0;
		while (outputGenerated < output.frames())
		{
			if (streamBuffer.count == 0)
			{
				streamBuffer.index = 0;

				const auto refillView = InterleavedBufferView<float, Channels>{&streamBuffer[0], Capacity};
				refillFn(refillView);

				// If the stream buffer is still empty, refill it with silence and use that as input
				// Ensures that the audio is always treated as being continuous
				if (streamBuffer.count == 0)
				{
					std::fill(streamBuffer.buffer.begin(), streamBuffer.buffer.end(), 0.f);
					streamBuffer.count = Capacity;
				}
			}

			const auto inputView
				= InterleavedBufferView<float, Channels>{&streamBuffer[streamBuffer.index], streamBuffer.count};
			const auto outputView = InterleavedBufferView<float, Channels>{
				output.framePtr(outputGenerated), output.frames() - outputGenerated};
			const auto result = process(inputView, outputView);

			streamBuffer.index += result.inputFramesUsed;
			streamBuffer.count -= result.inputFramesUsed;
			outputGenerated += result.outputGenerated;
		}
	}

	/**
	 * @brief Resets the internal resampler state.
	 * Useful when working with unreleated pieces of audio.
	 */
	void reset()
	{
		if ((m_error = src_reset(static_cast<SRC_STATE*>(m_state.get()))))
		{
#ifdef LMMS_DEBUG
			std::cerr << "AudioResampler: " << src_strerror(m_error) << '\n';
#endif
		}
	}

	/**
	 * @brief Sets the resampling ratio to `ratio`.
	 * @param ratio Output sample rate divided by input sample rate.
	 */
	void setRatio(double ratio) { m_ratio = ratio; }

	/**
	 * @brief Sets the resampling ratio to `output / input`.
	 * @param input Input sample rate.
	 * @param output Output sample rate.
	 */
	void setRatio(sample_rate_t input, sample_rate_t output) { m_ratio = static_cast<double>(output) / input; }

	//! @returns the resampling ratio.
	auto ratio() const -> double { return m_ratio; }

	//! @returns the number of channels expected by the resampler.
	constexpr auto channels() const -> ch_cnt_t { return Channels; }

private:
	struct LMMS_EXPORT StateDeleter
	{
		void operator()(SRC_STATE* state) { src_delete(state); }
	};

	std::unique_ptr<SRC_STATE*, StateDeleter> m_state;
	double m_ratio = 1.0;
	int m_error = 0;
};

} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
