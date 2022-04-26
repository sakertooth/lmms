/*
 * SampleBufferV2.cpp - container class for immutable sample data
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

#include "SampleBufferV2.h"
#include "AudioEngine.h"
#include "Engine.h"

#include <QFile>
#include <sndfile.h>

SampleBufferV2::SampleBufferV2(const QString &audioFilePath)
{
	//TODO: check if buffer is in the cache already before reloading it into memory

	auto audioFile = QFile(audioFilePath);
	if (!audioFile.open(QIODevice::ReadOnly)) 
	{
		throw std::runtime_error("Could not open file " + audioFilePath.toStdString());
	}

	SF_INFO sfInfo;
	sfInfo.format = 0;
	auto sndFile = std::make_unique<SNDFILE>(sf_open_fd(audioFile.handle(), SFM_READ, &sfInfo, false),
		[&](SNDFILE *sndFile)
		{	sf_close(sndFile);
			audioFile.close(); });

	if (!sndFile)
	{
		throw std::runtime_error(sf_strerror(sndFile.get()));
	}

	sf_count_t numSamples = sfInfo.frames * sfInfo.channels;
	m_data->resize(numSamples);

	sf_count_t framesRead = sf_read_float(sndFile.get(), m_data->data(), numSamples);
	if (framesRead == numSamples)
	{
		m_numChannels = sfInfo.channels;
		m_sampleRate = sfInfo.samplerate;
		m_filePath = audioFilePath;

		//TODO: add to the cache
	}
	else
	{
		throw std::runtime_error(sf_strerror(sndFile.get()));
	}
}

SampleBufferV2::SampleBufferV2(sampleFrame *data, const std::size_t numFrames) :
	m_data(std::make_unique<std::vector<sample_t>>(data->data(), data->data() + numFrames)),
	m_sampleRate(Engine::audioEngine()->processingSampleRate()),
	m_numChannels(DEFAULT_CHANNELS) {}

SampleBufferV2::SampleBufferV2(const std::size_t numFrames, ch_cnt_t numChannels) :
	m_data(std::make_unique<std::vector<sample_t>>(numFrames * numChannels)),
	m_sampleRate(Engine::audioEngine()->processingSampleRate()),
	m_numChannels(m_numChannels) {}

const std::vector<sample_t> &SampleBufferV2::data() const
{
	return *m_data;
}

sample_rate_t SampleBufferV2::sampleRate() const
{
	return m_sampleRate;
}

ch_cnt_t SampleBufferV2::numChannels() const
{
	return m_numChannels;
}

const QString &SampleBufferV2::filePath() const
{
	return m_filePath;
}