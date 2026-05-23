/*
 * Sample.cpp
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

#include "Sample.h"
#include "AudioChannelMatrix.h"

namespace lmms {

Sample::Sample(InterleavedBufferView<const float> buffer, sample_rate_t rate)
	: m_buffer{buffer}
	, m_sampleRate{rate}
	, m_endFrame{buffer.frames()}
	, m_loopEndFrame{buffer.frames()}
{
}

f_cnt_t Sample::play(InterleavedBufferView<float> output)
{
	const auto channelConversionMatrix = fetchChannelMatrix(m_buffer.channels(), output.channels());
	if (channelConversionMatrix == std::nullopt) { return 0; }

	std::fill_n(output.data(), output.frames() * output.channels(), 0.f);

	return std::visit([&](auto&& matrix)
	{
		for (f_cnt_t i = 0; i < output.frames(); ++i)
		{
			switch (m_loopMode)
			{
			case LoopMode::Off:
				if (m_frameIndex < 0 || m_frameIndex >= m_endFrame) { return i; }
				break;
			case LoopMode::On:
				if (m_frameIndex < m_loopStartFrame && m_backwards)
				{
					m_frameIndex = m_loopEndFrame - 1;
				}
				else if (m_frameIndex >= m_loopEndFrame) { m_frameIndex = m_loopStartFrame; }
				break;
			case LoopMode::PingPong:
				if (m_frameIndex < m_loopStartFrame && m_backwards)
				{
					m_frameIndex = m_loopStartFrame;
					m_backwards = false;
				}
				else if (m_frameIndex >= m_loopEndFrame)
				{
					m_frameIndex = m_loopEndFrame - 1;
					m_backwards = true;
				}
				break;
			default:
				break;
			}

			const auto index = static_cast<double>(m_reversed ? m_buffer.frames() - m_frameIndex - 1 : m_frameIndex);

			for (auto srcChannel = 0; srcChannel < m_buffer.channels(); ++srcChannel)
			{
				const auto sample = interpolate(index, srcChannel);
				for (auto dstChannel = 0; dstChannel < output.channels(); ++dstChannel)
				{
					output[i][dstChannel] += sample * matrix[srcChannel][dstChannel] * m_amplification;
				}
			}

			m_backwards ? m_frameIndex -= m_phase : m_frameIndex += m_phase;
		}
		return output.frames();
	}, channelConversionMatrix.value());
}

auto Sample::duration() const -> std::chrono::milliseconds
{
	const auto duration = (m_endFrame - m_startFrame) / static_cast<float>(m_sampleRate) * 1000;
	return std::chrono::milliseconds{static_cast<int>(duration)};
}

void Sample::setBuffer(InterleavedBufferView<const float> buffer, sample_rate_t rate)
{
	if (m_buffer.data() == buffer.data() && m_buffer.frames() == buffer.frames()
		&& m_buffer.channels() == buffer.channels() && m_sampleRate == rate)
	{
		return;
	}

	m_buffer = buffer;
	m_sampleRate = rate;

	m_startFrame = std::clamp(m_startFrame, f_cnt_t{0}, buffer.frames());
	m_endFrame = std::clamp(m_endFrame, f_cnt_t{0}, buffer.frames());
	m_frameIndex = std::clamp(m_frameIndex, static_cast<double>(m_startFrame), static_cast<double>(m_endFrame));
	m_loopStartFrame = std::clamp(m_loopStartFrame, m_startFrame, m_endFrame);
	m_loopEndFrame = std::clamp(m_loopEndFrame, m_startFrame, m_endFrame);
}

float Sample::interpolate(double index, ch_cnt_t channel)
{
	switch (m_interpolationMode)
	{
	case InterpolationMode::ZeroOrderHold:
		return m_buffer[static_cast<f_cnt_t>(index)][channel];
	case InterpolationMode::Linear:
	{
		const auto frameIndex = static_cast<f_cnt_t>(index);
		const auto t = index - frameIndex;
		const auto a = m_buffer[frameIndex][channel];
		const auto b = frameIndex < m_buffer.frames() ? m_buffer[frameIndex + 1][channel] : 0.f;
		return std::lerp(a, b, t);
	}
	case InterpolationMode::Sinc:
	{
		// for now
		return m_buffer[static_cast<f_cnt_t>(index)][channel];
	}
	}

	return 0.f;
}

} // namespace lmms
