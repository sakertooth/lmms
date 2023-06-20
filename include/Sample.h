/*
 * Sample.h - State for container-class SampleBuffer
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

#ifndef LMMS_SAMPLE_H
#define LMMS_SAMPLE_H

#include "SampleBuffer.h"
#include "SamplePlaybackState.h"
#include "lmms_export.h"

class QPainter;
class QRect;

namespace lmms {
class LMMS_EXPORT Sample
{
public:
	enum class Loop
	{
		Off,
		On,
		PingPong
	};

	Sample(std::shared_ptr<SampleBuffer> buffer);
	template <typename... Args> static std::shared_ptr<Sample> createFromBuffer(Args&&... args)
	{
		const auto buffer = std::make_shared<SampleBuffer>(std::forward<Args>(args)...);
		return std::make_shared<Sample>(buffer);
	}

	bool play(sampleFrame* dst, SamplePlaybackState* state, int numFrames, float freq = DefaultBaseFreq,
		Loop loopMode = Loop::Off);
	auto visualize(QPainter& p, const QRect& dr, int fromFrame = 0, int toFrame = 0) -> void;
	auto sampleDuration() const -> int;
	auto playbackSize() const -> int;

	static auto interpolationMargins() -> std::array<int, 5>&;
	auto buffer() const -> std::shared_ptr<SampleBuffer>;
	auto startFrame() const -> int;
	auto endFrame() const -> int;
	auto loopStartFrame() const -> int;
	auto loopEndFrame() const -> int;
	auto amplification() const -> float;
	auto frequency() const -> float;
	auto reversed() const -> bool;

	void setStartFrame(int frame);
	void setEndFrame(int frame);
	void setLoopStartFrame(int frame);
	void setLoopEndFrame(int frame);
	void setAllPointFrames(int start, int end, int loopStart, int loopEnd);
	void setAmplification(float amplification);
	void setFrequency(float frequency);
	void setReversed(bool reversed);

private:
	auto playSampleRange(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio = 1.0f)
		-> int;
	auto playSampleRangeBackwards(
		SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio = 1.0f) -> int;
	auto playSampleRangeLoop(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio = 1.0f)
		-> int;
	auto playSampleRangePingPong(
		SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio = 1.0f) -> int;
	auto resampleSampleRange(SRC_STATE* state, sampleFrame* src, int numInputFrames, sampleFrame* dst,
		int numOutputFrames, double ratio) -> SRC_DATA;
	void amplifySampleRange(sampleFrame* src, int numFrames);

	std::shared_ptr<SampleBuffer> m_buffer = std::make_shared<SampleBuffer>();
	std::atomic<int> m_startFrame;
	std::atomic<int> m_endFrame;
	std::atomic<int> m_loopStartFrame;
	std::atomic<int> m_loopEndFrame;
	std::atomic<float> m_amplification = 1.0f;
	std::atomic<float> m_frequency = DefaultBaseFreq;
	std::atomic<bool> m_reversed = false;
};
} // namespace lmms
#endif // LMMS_SAMPLE_H
