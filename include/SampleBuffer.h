/*
 * SampleBuffer.h - container-class SampleBuffer
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

#ifndef LMMS_SAMPLE_BUFFER_H
#define LMMS_SAMPLE_BUFFER_H

#include <QString>
#include <memory>
#include <vector>

#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleBuffer
{
public:
	SampleBuffer() = default;
	SampleBuffer(std::vector<SampleFrame> data, sample_rate_t sampleRate, const QString& path = "");
	SampleBuffer(const SampleFrame* data, f_cnt_t numFrames, sample_rate_t sampleRate, const QString& path = "");

	auto toBase64() const -> QString;
	auto empty() const -> bool { return m_data.empty(); }

	auto data() const -> const SampleFrame* { return m_data.data(); }
	auto sampleRate() const -> sample_rate_t { return m_sampleRate; }
	auto numFrames() const -> f_cnt_t { return m_data.size(); }
	auto path() const -> const QString& { return m_path; }

	static auto emptyBuffer() -> std::shared_ptr<const SampleBuffer>;

	static auto fromFile(const QString& path) -> std::shared_ptr<const SampleBuffer>;
	static auto fromBase64(const QString& str, sample_rate_t sampleRate) -> std::shared_ptr<const SampleBuffer>;

private:
	std::vector<SampleFrame> m_data;
	sample_rate_t m_sampleRate = 0;
	QString m_path;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
