/*
 * AudioResampler.h - wrapper around libsamplerate
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include <samplerate.h>

#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {

class LMMS_EXPORT AudioResampler
{
public:
	struct ProcessResult
	{
		int error;
		long inputFramesUsed;
		long outputFramesGenerated;
	};

	//! Creates a resampler that uses the resample quality set in `AudioQuality`
	//! with `channels` channels.
	AudioResampler(int channels = DEFAULT_CHANNELS);

	//! Creates a resampler that uses the libsamplerate converter type  with
	//! with `channels` channels. 
	AudioResampler(int interpolationMode, int channels = DEFAULT_CHANNELS);

	AudioResampler(const AudioResampler&);
	AudioResampler(AudioResampler&&) noexcept;
	~AudioResampler();

	AudioResampler& operator=(const AudioResampler&);
	AudioResampler& operator=(AudioResampler&&) noexcept;

	auto resample(const float* in, long inputFrames, float* out, long outputFrames, double ratio) -> ProcessResult;
	auto interpolationMode() const -> int { return m_interpolationMode; }
	auto channels() const -> int { return m_channels; }

private:
	int m_interpolationMode = -1;
	int m_channels = 0;
	int m_error = 0;
	bool m_useAudioQuality = false;
	SRC_STATE* m_state = nullptr;
};
} // namespace lmms

#endif // LMMS_AUDIO_RESAMPLER_H
