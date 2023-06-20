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

#include <QPainter>
#include <QRect>
#include <algorithm>
#include <iostream>

#include "SamplePlaybackState.h"
#include "lmms_basics.h"

namespace lmms {
// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
static auto s_interpolationMargins = std::array<int, 5>{64, 64, 64, 4, 4};

Sample::Sample(std::shared_ptr<SampleBuffer> buffer)
	: m_buffer(buffer)
	, m_startFrame(0)
	, m_endFrame(static_cast<int>(m_buffer->size()))
	, m_loopStartFrame(0)
	, m_loopEndFrame(static_cast<int>(m_buffer->size()))
{
}

bool Sample::play(sampleFrame* dst, SamplePlaybackState* state, int numFrames, float freq, Loop loopMode)
{
	auto numFramesPlayed = 0;
	const auto resampleRatio
		= m_frequency / freq * Engine::audioEngine()->processingSampleRate() / m_buffer->sampleRate();

	switch (loopMode)
	{
	case Loop::Off:
		numFramesPlayed = playSampleRange(state, numFrames, dst, resampleRatio);
		break;
	case Loop::On:
		numFramesPlayed = playSampleRangeLoop(state, numFrames, dst, resampleRatio);
		break;
	case Loop::PingPong:
		numFramesPlayed = playSampleRangePingPong(state, numFrames, dst, resampleRatio);
		break;
	default:
		return false;
	}

	if (src_error(state->m_resamplingData)) { return false; }
	if (numFramesPlayed <= 0) { return false; }

	amplifySampleRange(dst, numFramesPlayed);
	std::fill_n(dst + numFramesPlayed, numFrames - numFramesPlayed, sampleFrame{0, 0});
	return true;
}

int Sample::playSampleRange(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio)
{
	if (state->m_frameIndex >= m_endFrame) { return false; }
	const auto playFrame = std::max<int>(m_startFrame, state->m_frameIndex);
	const auto numFramesToCopy = std::min<int>(
		numFrames / resampleRatio + s_interpolationMargins[state->m_interpolationMode], m_endFrame - playFrame);

	auto buffer = std::vector<sampleFrame>(numFramesToCopy);
	m_reversed ? std::copy_n(m_buffer->rbegin() + playFrame, buffer.size(), buffer.begin())
			   : std::copy_n(m_buffer->begin() + playFrame, buffer.size(), buffer.begin());

	const auto resample
		= resampleSampleRange(state->m_resamplingData, buffer.data(), buffer.size(), dst, numFrames, resampleRatio);
	state->m_frameIndex = playFrame + resample.input_frames_used;
	return resample.output_frames_gen;
}

int Sample::playSampleRangeBackwards(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio)
{
	if (state->m_frameIndex < m_startFrame) { return false; }
	const auto playFrame = std::min<int>(m_endFrame, state->m_frameIndex);
	const auto numFramesToCopy = std::min<int>(
		numFrames / resampleRatio + s_interpolationMargins[state->m_interpolationMode], playFrame - m_startFrame);

	auto buffer = std::vector<sampleFrame>(numFramesToCopy);
	m_reversed ? std::copy_n(m_buffer->begin() + m_endFrame - playFrame, buffer.size(), buffer.begin())
			   : std::copy_n(m_buffer->rbegin() + m_endFrame - playFrame, buffer.size(), buffer.begin());

	const auto resample
		= resampleSampleRange(state->m_resamplingData, buffer.data(), buffer.size(), dst, numFrames, resampleRatio);
	state->m_frameIndex = playFrame - resample.input_frames_used;
	return resample.output_frames_gen;
}

int Sample::playSampleRangeLoop(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio)
{
	if (state->m_frameIndex >= m_loopEndFrame) { state->m_frameIndex = m_loopStartFrame; }

	auto numFramesCopied = 0;
	while (numFramesCopied != numFrames)
	{
		const auto framesToPlay = std::min(numFrames - numFramesCopied, m_loopEndFrame - state->m_frameIndex);
		numFramesCopied += playSampleRange(state, framesToPlay, dst + numFramesCopied, resampleRatio);
		if (state->m_frameIndex == m_loopEndFrame) { state->m_frameIndex = m_loopStartFrame; }
	}

	return numFramesCopied;
}

int Sample::playSampleRangePingPong(SamplePlaybackState* state, int numFrames, sampleFrame* dst, float resampleRatio)
{
	if (state->m_frameIndex >= m_loopEndFrame)
	{
		state->m_frameIndex = m_loopEndFrame - 1;
		state->m_backwards = true;
	}

	auto numFramesCopied = 0;
	while (numFramesCopied != numFrames)
	{
		auto framesToPlay = std::min(numFrames - numFramesCopied,
			state->m_backwards ? state->m_frameIndex - m_loopStartFrame : m_loopEndFrame - state->m_frameIndex);

		numFramesCopied += state->m_backwards
			? playSampleRangeBackwards(state, framesToPlay, dst + numFramesCopied, resampleRatio)
			: playSampleRange(state, framesToPlay, dst + numFramesCopied, resampleRatio);

		if (state->m_frameIndex == m_loopEndFrame && !state->m_backwards) { state->m_backwards = true; }
		else if (state->m_frameIndex == m_loopStartFrame && state->m_backwards) { state->m_backwards = false; }
	}

	return numFramesCopied;
}

SRC_DATA Sample::resampleSampleRange(
	SRC_STATE* state, sampleFrame* src, int numInputFrames, sampleFrame* dst, int numOutputFrames, double ratio)
{
	auto srcData = SRC_DATA{};
	srcData.data_in = &src[0][0];
	srcData.data_out = &dst[0][0];
	srcData.input_frames = numInputFrames;
	srcData.output_frames = numOutputFrames;
	srcData.src_ratio = ratio;
	srcData.end_of_input = 0;
	src_process(state, &srcData);
	return srcData;
}

void Sample::amplifySampleRange(sampleFrame* src, int numFrames)
{
	for (int i = 0; i < numFrames; ++i)
	{
		src[i][0] *= m_amplification;
		src[i][1] *= m_amplification;
	}
}

void Sample::visualize(QPainter& p, const QRect& dr, int fromFrame, int toFrame)
{
	const auto numFrames = static_cast<int>(m_buffer->size());
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

auto Sample::interpolationMargins() -> std::array<int, 5>&
{
	return s_interpolationMargins;
}

auto Sample::sampleDuration() const -> int
{
	return m_buffer->sampleRate() > 0 ? static_cast<double>(m_endFrame - m_startFrame) / m_buffer->sampleRate() * 1000
									  : 0;
}

auto Sample::playbackSize() const -> int
{
	return m_buffer->sampleRate() > 0
		? m_buffer->size() * Engine::audioEngine()->processingSampleRate() / m_buffer->sampleRate()
		: 0;
}

auto Sample::buffer() const -> std::shared_ptr<SampleBuffer>
{
	return m_buffer;
}

auto Sample::startFrame() const -> int
{
	return m_startFrame;
}

auto Sample::endFrame() const -> int
{
	return m_endFrame;
}

auto Sample::loopStartFrame() const -> int
{
	return m_loopStartFrame;
}

auto Sample::loopEndFrame() const -> int
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

auto Sample::setStartFrame(int frame) -> void
{
	m_startFrame = frame;
}

auto Sample::setEndFrame(int frame) -> void
{
	m_endFrame = frame;
}

auto Sample::setLoopStartFrame(int frame) -> void
{
	m_loopStartFrame = frame;
}

auto Sample::setLoopEndFrame(int frame) -> void
{
	m_loopEndFrame = frame;
}

auto Sample::setAllPointFrames(int start, int end, int loopStart, int loopEnd) -> void
{
	m_startFrame = start;
	m_endFrame = end;
	m_loopStartFrame = loopStart;
	m_loopEndFrame = loopEnd;
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
