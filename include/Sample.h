/*
 * Sample.h
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

#ifndef LMMS_SAMPLE_H
#define LMMS_SAMPLE_H

#include <chrono>

#include "AudioBufferView.h"
#include "Note.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT Sample
{
public:
	enum class LoopMode
	{
		Off,
		On,
		PingPong
	};

	enum class InterpolationMode
	{
		ZeroOrderHold,
		Linear,
		Sinc
	};

	Sample(InterleavedBufferView<const float> buffer, sample_rate_t rate);

	auto play(InterleavedBufferView<float> output) -> f_cnt_t;

	auto frames() const -> f_cnt_t { return m_buffer.frames(); }
	auto duration() const -> std::chrono::milliseconds;
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }

	auto frameIndex() const -> f_cnt_t { return m_frameIndex; }
	auto startFrame() const -> f_cnt_t { return m_startFrame; }
	auto endFrame() const -> f_cnt_t { return m_endFrame; }

	auto loopStartFrame() const -> f_cnt_t { return m_loopStartFrame; }
	auto loopEndFrame() const -> f_cnt_t { return m_loopEndFrame; }
	auto loopMode() const -> LoopMode { return m_loopMode; }

	auto amplification() const -> float { return m_amplification; }
	auto reversed() const -> bool { return m_reversed; }
	auto backwards() const -> bool { return m_backwards; }

	void setBuffer(InterleavedBufferView<const float> buffer, sample_rate_t rate);
	void setFrameIndex(f_cnt_t frame) { m_frameIndex = frame; }

	void setStartFrame(f_cnt_t frame) { m_startFrame = frame; }
	void setEndFrame(f_cnt_t frame) { m_endFrame = frame; }

	void setLoopStartFrame(f_cnt_t frame) { m_loopStartFrame = frame; }
	void setLoopEndFrame(f_cnt_t frame) { m_loopEndFrame = frame; }

	void setLoopMode(LoopMode mode) { m_loopMode = mode; }
	void setInterpolationMode(InterpolationMode mode) { m_interpolationMode = mode; }

	void setAmplification(float amplification) { m_amplification = amplification; }
	void setPhase(double phase) { m_phase = phase; }
	void setReversed(bool reversed) { m_reversed = reversed; }
	void setBackwards(bool backwards) { m_backwards = backwards; }

private:
	float interpolate(double frameIndex, ch_cnt_t channelIndex);

	InterleavedBufferView<const float> m_buffer;
	sample_rate_t m_sampleRate = 0;
	f_cnt_t m_startFrame = 0;
	f_cnt_t m_endFrame = 0;
	f_cnt_t m_loopStartFrame = 0;
	f_cnt_t m_loopEndFrame = 0;
	LoopMode m_loopMode = LoopMode::Off;
	InterpolationMode m_interpolationMode = InterpolationMode::Linear;
	float m_amplification = 1.0f;
	double m_frameIndex = 0;
	double m_phase = 1.0;
	bool m_reversed = false;
	bool m_backwards = false;
};
} // namespace lmms
#endif
