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

#include <QPainter>
#include <QRect>
#include <memory>
#include <samplerate.h>

#include "AudioEngine.h"
#include "Engine.h"
#include "Note.h"
#include "SampleBuffer.h"
#include "SamplePlaybackState.h"
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT Sample
{
public:
	enum class LoopMode
	{
		LoopOff,
		LoopOn,
		LoopPingPong
	};

	Sample(std::shared_ptr<SampleBuffer> buffer);

	template <typename... Args> static std::shared_ptr<Sample> createFromBuffer(Args&&... args)
	{
		const auto buffer = std::make_shared<SampleBuffer>(std::forward<Args>(args)...);
		return std::make_shared<Sample>(buffer);
	}

	auto play(sampleFrame* dst, SamplePlaybackState* state, fpp_t frames, float freq,
		LoopMode loopMode = LoopMode::LoopOff) -> bool;

	//! TODO: Should be moved to its own QWidget
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

	auto setStartFrame(int frame) -> void;
	auto setEndFrame(int frame) -> void;
	auto setLoopStartFrame(int frame) -> void;
	auto setLoopEndFrame(int frame) -> void;
	auto setAllPointFrames(int startFrame, int endFrame, int loopStartFrame, int loopEndFrame) -> void;
	auto setAmplification(float amplification) -> void;
	auto setFrequency(float frequency) -> void;
	auto setReversed(bool reversed) -> void;

private:
	auto getSampleFragment(int index, int frames, LoopMode loopMode, bool* backwards, int loopStart, int loopEnd,
		int end) const -> std::vector<sampleFrame>;
	auto advance(int playFrame, int frames, LoopMode loopMode, SamplePlaybackState* state) -> int;
	auto getLoopedIndex(int index, int startFrame, int endFrame) const -> int;
	auto getPingPongIndex(int index, int startFrame, int endFrame) const -> int;

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
