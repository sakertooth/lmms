/*
 * Sample.cpp - State for container-class SampleBuffer
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

#include "Sample.h"

#include <cassert>
#include <iostream>
#include <samplerate.h>

#include "AudioEngine.h"
#include "Engine.h"
#include "SampleBuffer.h"
#include "lmms_basics.h"

namespace lmms {
// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
static auto s_interpolationMargins = std::array<f_cnt_t, 5>{64, 64, 64, 4, 4};

Sample::Sample(std::shared_ptr<SampleBuffer> buffer)
	: m_buffer(buffer)
	, m_endFrame(static_cast<f_cnt_t>(m_buffer->size()))
	, m_loopEndFrame(static_cast<f_cnt_t>(m_buffer->size()))
{
}

std::shared_ptr<Sample> Sample::makeUniqueSample(const std::shared_ptr<Sample>& sample)
{
	return std::make_shared<Sample>(sample->m_buffer);
}

auto Sample::play(sampleFrame* dst, SamplePlaybackState* state, fpp_t frames, float freq, LoopMode loopMode) -> bool
{
	if (m_endFrame == 0 || frames == 0) { return false; }

	const auto freqFactor = freq / m_frequency * m_buffer->sampleRate() / Engine::audioEngine()->processingSampleRate();
	const auto totalFramesForCurrentPitch = static_cast<f_cnt_t>((m_endFrame - m_startFrame) / freqFactor);
	if (totalFramesForCurrentPitch == 0) { return false; }

	auto playFrame = std::max(state->m_frameIndex, startFrame());
	if (loopMode == LoopMode::LoopOff)
	{
		if (playFrame >= m_endFrame) { return false; }
	}
	else if (loopMode == LoopMode::LoopOn) { playFrame = getLoopedIndex(playFrame, m_loopStartFrame, m_loopEndFrame); }
	else { playFrame = getPingPongIndex(playFrame, m_loopStartFrame, m_loopEndFrame); }

	const auto fragmentSize
		= static_cast<f_cnt_t>(frames * freqFactor) + s_interpolationMargins[state->interpolationMode()];
	auto pingPongBackwards = state->isBackwards();

	if (freqFactor != 1.0 || state->m_varyingPitch)
	{
		const auto sampleFragment = getSampleFragment(
			playFrame, fragmentSize, loopMode, &pingPongBackwards, m_loopStartFrame, m_loopEndFrame, m_endFrame);
		const auto srcData = resampleFrameBlock(
			state->m_resamplingData, &sampleFragment[0][0], dst->data(), fragmentSize, frames, 1.0 / freqFactor);
		playFrame = advance(playFrame, srcData.input_frames_used, loopMode, state);
	}
	else
	{
		const auto sampleFragment = getSampleFragment(
			playFrame, frames, loopMode, &pingPongBackwards, m_loopStartFrame, m_loopEndFrame, m_endFrame);
		std::copy_n(sampleFragment.begin(), frames, dst);
		playFrame = advance(playFrame, frames, loopMode, state);
	}

	state->setBackwards(pingPongBackwards);
	state->setFrameIndex(playFrame);

	for (fpp_t i = 0; i < frames; ++i)
	{
		dst[i][0] *= m_amplification;
		dst[i][1] *= m_amplification;
	}

	return true;
}

auto Sample::advance(f_cnt_t playFrame, f_cnt_t frames, LoopMode loopMode, SamplePlaybackState* state) -> f_cnt_t
{
	switch (loopMode)
	{
	case LoopMode::LoopOff:
		return playFrame + frames;
	case LoopMode::LoopOn:
		return frames + getLoopedIndex(playFrame, m_loopStartFrame, m_loopEndFrame);
	case LoopMode::LoopPingPong: {
		f_cnt_t left = frames;
		if (state->isBackwards())
		{
			playFrame -= frames;
			if (playFrame < m_loopStartFrame)
			{
				left -= m_loopStartFrame - playFrame;
				playFrame = m_loopStartFrame;
			}
			else { left = 0; }
		}

		return left + getPingPongIndex(playFrame, m_loopStartFrame, m_loopEndFrame);
	}
	default:
		return playFrame;
	}
}

std::vector<sampleFrame> Sample::getSampleFragment(f_cnt_t currentFrame, f_cnt_t numFramesRequested, LoopMode loopMode,
	bool* backwards, f_cnt_t loopStart, f_cnt_t loopEnd, f_cnt_t endFrame) const
{
	auto out = std::vector<sampleFrame>(numFramesRequested);
	if (loopMode == LoopMode::LoopOff)
	{
		// Specify the number of frames to copy, starting at index
		// If there are not enough frames to copy, copy the remaining frames
		// (endFrame - currentFrame)
		const auto numFramesToCopy = std::min(numFramesRequested, endFrame - currentFrame);
		assert(numFramesToCopy >= 0);

		if (m_reversed) { std::copy_n(m_buffer->rbegin() + currentFrame, numFramesToCopy, out.begin()); }
		else { std::copy_n(m_buffer->begin() + currentFrame, numFramesToCopy, out.begin()); }
	}
	else if (loopMode == LoopMode::LoopOn || loopMode == LoopMode::LoopPingPong)
	{
		// Will be used to track the current play position
		// and how many frames we copied while looping
		auto playPosition = currentFrame;
		auto numFramesCopied = 0;

		// Move playPosition to loopStart if playPosition has past or is currently at loopEnd
		if (playPosition >= loopEnd) { playPosition = loopStart; }

		while (numFramesCopied != numFramesRequested)
		{
			// If we do not have enough frames to copy, then one of these conditions must be true:
			// 1. playPosition + numFramesRequested > loopEnd for LoopOn mode and LoopPingPong mode (non-backwards),
			// 2. playPosition - numFramesRequested < loopStart for LoopPingPong mode (backwards),

			// If any of the previous conditions are true, then we should only copy the remaining frames
			// we have in the following ranges:
			// 1. loopEnd - playPosition for LoopOn mode and LoopPingPongMode (non-backwards)
			// 2. playPosition - loopStart for LoopPingPong mode (backwards).

			// Otherwise, we want to copy numFramesRequested frames minus how many frames we already copied

			const auto remainingFrames = (*backwards && loopMode == LoopMode::LoopPingPong) ? playPosition - loopStart
																							: loopEnd - playPosition;
			const auto numFramesToCopy = std::min(numFramesRequested - numFramesCopied, remainingFrames);
			assert(numFramesToCopy >= 0);

			if (loopMode == LoopMode::LoopOn || (!*backwards && loopMode == LoopMode::LoopPingPong))
			{
				if (m_reversed)
				{
					std::copy_n(m_buffer->rbegin() + playPosition, numFramesToCopy, out.begin() + numFramesCopied);
				}
				else { std::copy_n(m_buffer->begin() + playPosition, numFramesToCopy, out.begin() + numFramesCopied); }
			}
			else if (*backwards && loopMode == LoopMode::LoopPingPong)
			{
				auto distanceFromPlayPosition = std::distance(m_buffer->begin() + playPosition, m_buffer->end());

				if (m_reversed)
				{
					std::copy_n(
						m_buffer->begin() + distanceFromPlayPosition, numFramesToCopy, out.begin() + numFramesCopied);
				}
				else
				{
					std::copy_n(
						m_buffer->rbegin() + distanceFromPlayPosition, numFramesToCopy, out.begin() + numFramesCopied);
				}
			}

			playPosition += (*backwards && loopMode == LoopMode::LoopPingPong ? -numFramesToCopy : numFramesToCopy);
			numFramesCopied += numFramesToCopy;

			// Ensure that playPosition is in the valid range [0, loopEnd]
			assert(playPosition >= 0 && playPosition <= loopEnd);

			// numFramesCopied can only be less than or equal to numFramesRequested
			assert(numFramesCopied <= numFramesRequested);

			if (playPosition == loopEnd && loopMode == LoopMode::LoopOn) { playPosition = loopStart; }
			else if (playPosition == loopEnd && loopMode == LoopMode::LoopPingPong) { *backwards = true; }
			else if (playPosition == loopStart && *backwards && loopMode == LoopMode::LoopPingPong)
			{
				*backwards = false;
			}
		}
	}

	return out;
}

auto Sample::resampleFrameBlock(
	SRC_STATE* state, const float* in, float* out, int numInputFrames, int numOutputFrames, double ratio) -> SRC_DATA
{
	auto srcData = SRC_DATA{};
	srcData.data_in = in;
	srcData.data_out = out;
	srcData.input_frames = numInputFrames;
	srcData.output_frames = numOutputFrames;
	srcData.src_ratio = ratio;
	srcData.end_of_input = 0;

	if (const auto error = src_process(state, &srcData); error != 0)
	{
#ifdef LMMS_DEBUG
		std::cerr << "SampleBuffer: error while resampling: " << src_strerror(error) << '\n';
#endif
	}

	if (srcData.output_frames_gen > numOutputFrames)
	{
#ifdef LMMS_DEBUG
		std::cerr << "SampleBuffer: not enough frames: " << srcData.output_frames_gen << " / " << numOutputFrames
				  << '\n';
#endif
	}

	return srcData;
}

void Sample::visualize(QPainter& p, const QRect& dr, f_cnt_t fromFrame, f_cnt_t toFrame)
{
	const auto numFrames = static_cast<f_cnt_t>(m_buffer->size());
	if (numFrames == 0) { return; }

	const bool focusOnRange = toFrame <= numFrames && 0 <= fromFrame && fromFrame < toFrame;
	const int w = dr.width();
	const int h = dr.height();

	const int yb = h / 2 + dr.y();
	const float ySpace = h * 0.5f;
	const int nbFrames = focusOnRange ? toFrame - fromFrame : numFrames;

	const double fpp = std::max(1., static_cast<double>(nbFrames) / w);
	// There are 2 possibilities: Either nbFrames is bigger than
	// the width, so we will have width points, or nbFrames is
	// smaller than the width (fpp = 1) and we will have nbFrames
	// points
	const int totalPoints = nbFrames > w ? w : nbFrames;
	std::vector<QPointF> fEdgeMax(totalPoints);
	std::vector<QPointF> fEdgeMin(totalPoints);
	std::vector<QPointF> fRmsMax(totalPoints);
	std::vector<QPointF> fRmsMin(totalPoints);
	int curPixel = 0;
	const int xb = dr.x();
	const int first = focusOnRange ? fromFrame : 0;
	const int last = focusOnRange ? toFrame - 1 : numFrames - 1;
	// When the number of frames isn't perfectly divisible by the
	// width, the remaining frames don't fit the last pixel and are
	// past the visible area. lastVisibleFrame is the index number of
	// the last visible frame.
	const int visibleFrames = (fpp * w);
	const int lastVisibleFrame = focusOnRange ? fromFrame + visibleFrames - 1 : visibleFrames - 1;

	for (double frame = first; frame <= last && frame <= lastVisibleFrame; frame += fpp)
	{
		float maxData = -1;
		float minData = 1;

		auto rmsData = std::array<float, 2>{};

		// Find maximum and minimum samples within range
		for (int i = 0; i < fpp && frame + i <= last; ++i)
		{
			for (int j = 0; j < 2; ++j)
			{
				auto curData = m_buffer->data()[static_cast<int>(frame) + i][j];

				if (curData > maxData) { maxData = curData; }
				if (curData < minData) { minData = curData; }

				rmsData[j] += curData * curData;
			}
		}

		const float trueRmsData = (rmsData[0] + rmsData[1]) / 2 / fpp;
		const float sqrtRmsData = sqrt(trueRmsData);
		const float maxRmsData = std::clamp(sqrtRmsData, minData, maxData);
		const float minRmsData = std::clamp(-sqrtRmsData, minData, maxData);

		// If nbFrames >= w, we can use curPixel to calculate X
		// but if nbFrames < w, we need to calculate it proportionally
		// to the total number of points
		auto x = nbFrames >= w ? xb + curPixel : xb + ((static_cast<double>(curPixel) / nbFrames) * w);

		if (m_reversed) { x = w - 1 - x; }

		// Partial Y calculation
		auto py = ySpace * m_amplification;
		fEdgeMax[curPixel] = QPointF(x, (yb - (maxData * py)));
		fEdgeMin[curPixel] = QPointF(x, (yb - (minData * py)));
		fRmsMax[curPixel] = QPointF(x, (yb - (maxRmsData * py)));
		fRmsMin[curPixel] = QPointF(x, (yb - (minRmsData * py)));
		++curPixel;
	}

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fEdgeMax[i], fEdgeMin[i]);
	}

	p.setPen(p.pen().color().lighter(123));

	for (int i = 0; i < totalPoints; ++i)
	{
		p.drawLine(fRmsMax[i], fRmsMin[i]);
	}
}

auto Sample::interpolationMargins() -> std::array<f_cnt_t, 5>&
{
	return s_interpolationMargins;
}

auto Sample::sampleDuration() const -> int
{
	return m_buffer->sampleRate() > 0 ? static_cast<double>(m_endFrame - m_startFrame) / m_buffer->sampleRate() * 1000
									  : 0;
}

auto Sample::playbackSize() const -> f_cnt_t
{
	return m_buffer->sampleRate() > 0
		? m_buffer->size() * Engine::audioEngine()->processingSampleRate() / m_buffer->sampleRate()
		: 0;
}

f_cnt_t Sample::getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf) { return index; }
	const f_cnt_t loopLen = endf - startf;
	const f_cnt_t loopPos = (index - endf) % (loopLen * 2);

	return (loopPos < loopLen) ? endf - loopPos : startf + (loopPos - loopLen);
}

f_cnt_t Sample::getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf) { return index; }
	return startf + (index - startf) % (endf - startf);
}

auto Sample::buffer() const -> std::shared_ptr<SampleBuffer>
{
	return m_buffer;
}

auto Sample::startFrame() const -> f_cnt_t
{
	return m_startFrame;
}

auto Sample::endFrame() const -> f_cnt_t
{
	return m_endFrame;
}

auto Sample::loopStartFrame() const -> f_cnt_t
{
	return m_loopStartFrame;
}

auto Sample::loopEndFrame() const -> f_cnt_t
{
	return m_loopEndFrame;
}

auto Sample::amplification() const -> float
{
	return m_amplification;
}

auto Sample::frequency() const -> float
{
	return m_frequency;
}

auto Sample::reversed() const -> bool
{
	return m_reversed;
}

auto Sample::setStartFrame(f_cnt_t frame) -> void
{
	m_startFrame = frame;
}

auto Sample::setEndFrame(f_cnt_t frame) -> void
{
	m_endFrame = frame;
}

auto Sample::setLoopStartFrame(f_cnt_t frame) -> void
{
	m_loopStartFrame = frame;
}

auto Sample::setLoopEndFrame(f_cnt_t frame) -> void
{
	m_loopEndFrame = frame;
}

auto Sample::setAllPointFrames(f_cnt_t startFrame, f_cnt_t endFrame, f_cnt_t loopStartFrame, f_cnt_t loopEndFrame)
	-> void
{
	m_startFrame = startFrame;
	m_endFrame = endFrame;
	m_loopStartFrame = loopStartFrame;
	m_loopEndFrame = loopEndFrame;
}
auto Sample::setAmplification(float amplification) -> void
{
	m_amplification = amplification;
}

auto Sample::setFrequency(float frequency) -> void
{
	m_frequency = frequency;
}

auto Sample::setReversed(bool reversed) -> void
{
	m_reversed = reversed;
}
} // namespace lmms
