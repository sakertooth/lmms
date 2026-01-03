/*
 * SampleBuffer.cpp - container-class SampleBuffer
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

#include "SampleBuffer.h"
#include <cstring>

#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms {

SampleBuffer::SampleBuffer(const SampleFrame* data, size_t numFrames, sample_rate_t sampleRate, const QString& path)
	: m_data(data, data + numFrames)
	, m_sampleRate(sampleRate)
	, m_path(path)
{
}

SampleBuffer::SampleBuffer(std::vector<SampleFrame> data, sample_rate_t sampleRate, const QString& path)
	: m_data(std::move(data))
	, m_sampleRate(sampleRate)
	, m_path(path)
{
}

QString SampleBuffer::toBase64() const
{
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(SampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::emptyBuffer() -> std::shared_ptr<const SampleBuffer>
{
	static auto s_buffer = std::make_shared<const SampleBuffer>();
	return s_buffer;
}

auto SampleBuffer::fromFile(const QString& path) -> std::shared_ptr<const SampleBuffer>
{
	const auto absolutePath = PathUtil::toAbsolute(path);
	const auto decodedResult = SampleDecoder::decode(absolutePath);

	if (!decodedResult) { return SampleBuffer::emptyBuffer(); }

	auto& [data, sampleRate] = *decodedResult;
	return std::make_shared<SampleBuffer>(std::move(data), sampleRate, PathUtil::toShortestRelative(path));
}

auto SampleBuffer::fromBase64(const QString& str, sample_rate_t sampleRate) -> std::shared_ptr<const SampleBuffer>
{
	const auto bytes = QByteArray::fromBase64(str.toUtf8());
	auto buffer = std::vector<SampleFrame>(bytes.size() / sizeof(SampleFrame));
	std::memcpy(buffer.data(), bytes, buffer.size() * sizeof(SampleFrame));
	return std::make_shared<SampleBuffer>(std::move(buffer), sampleRate);
}

} // namespace lmms
