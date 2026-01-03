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

SampleBuffer::SampleBuffer(
	std::unique_ptr<SampleFrame[]> data, f_cnt_t numFrames, sample_rate_t sampleRate, const QString& path)
	: m_data(std::move(data))
	, m_numFrames(numFrames)
	, m_sampleRate(sampleRate)
	, m_path(path)
{
}

QString SampleBuffer::toBase64() const
{
	const auto data = reinterpret_cast<const char*>(m_data.get());
	const auto size = static_cast<int>(m_numFrames * sizeof(SampleFrame));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

auto SampleBuffer::fromFile(const QString& path) -> std::optional<SampleBuffer>
{
	const auto absolutePath = PathUtil::toAbsolute(path);
	auto decodedResult = SampleDecoder::decode(absolutePath);

	if (!decodedResult) { return std::nullopt; }

	auto& [data, numFrames, sampleRate] = *decodedResult;
	return SampleBuffer{std::move(data), numFrames, sampleRate, PathUtil::toShortestRelative(path)};
}

auto SampleBuffer::fromBase64(const QString& str, sample_rate_t sampleRate) -> std::optional<SampleBuffer>
{
	const auto bytes = QByteArray::fromBase64(str.toUtf8());
	const auto numFrames = bytes.size() / sizeof(SampleFrame);
	auto buffer = std::make_unique<SampleFrame[]>(numFrames);
	std::memcpy(buffer.get(), bytes, numFrames * sizeof(SampleFrame));
	return SampleBuffer{std::move(buffer), numFrames, sampleRate};
}

} // namespace lmms
