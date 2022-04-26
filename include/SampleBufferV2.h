/*
 * SampleBufferV2.h - container class for immutable sample data
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#ifndef SAMPLE_BUFFER_V2_H
#define SAMPLE_BUFFER_V2_H

#include <memory>
#include <vector>
#include "lmms_basics.h"

#include <QString>

class SampleBufferV2
{
public:
	SampleBufferV2();
	SampleBufferV2(const QString& audioFilePath);
	SampleBufferV2(sampleFrame* data, const std::size_t numFrames);
	SampleBufferV2(const std::size_t numFrames, ch_cnt_t m_numChannels);

	const std::vector<sample_t>& data() const;
	sample_rate_t sampleRate() const;
	ch_cnt_t numChannels() const;
	const QString& filePath() const;

private:
	
private:
	//sample_t and not sampleFrame to maintain a minimum amount of memory for mono tracks
	std::shared_ptr<std::vector<sample_t>> m_data;
	
	sample_rate_t m_sampleRate;
	ch_cnt_t m_numChannels;

	//TODO: C++17 and above: use std::optional<QString>
	QString m_filePath;
};

#endif