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
#include "Oscillator.h"

#include <algorithm>
#include <iostream>

#include <QFile>
#include <QMessageBox>

#include <sndfile.h>

#include "AudioEngine.h"
#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"
#include "endian_handling.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "PathUtil.h"

namespace lmms
{

SampleBuffer::SampleBuffer(const sampleFrame* data, int numFrames, int sampleRate) :
	m_data(data, data + numFrames),
	m_originalData(data, data + numFrames),
	m_sampleRate(sampleRate),
	m_originalSampleRate(sampleRate)
{
	const auto engineRate = Engine::audioEngine()->processingSampleRate();
	if (sampleRate != static_cast<int>(engineRate))
	{
		resample(engineRate);
	}
}

SampleBuffer::SampleBuffer(const QString& audioFile)
{
	if (audioFile.isEmpty())
	{
		throw std::runtime_error{"Failure loading audio file: Audio file path is empty."};
	}

	auto resolvedFileName = PathUtil::toAbsolute(PathUtil::toShortestRelative(audioFile));
	if (QFileInfo{resolvedFileName}.suffix() == "ds")
	{
		// Note: DrumSynth files aren't checked for file limits since we are using sndfile to detect them.
		// In the future, checking for limits may become unnecessary anyways, so this seems fine for now.
		decodeSampleDS(resolvedFileName);
	}
	else if (!fileExceedsLimits(resolvedFileName))
	{
		decodeSampleSF(resolvedFileName);
	}
}

SampleBuffer::SampleBuffer(const QByteArray& base64)
{
	// TODO: Replace with non-Qt equivalent
	auto base64Data = base64.toBase64();
	auto sampleBufferData = reinterpret_cast<SampleBuffer*>(base64Data.data());

	m_audioFile = std::move(sampleBufferData->m_audioFile);
	m_data = std::move(sampleBufferData->m_data);
	m_sampleRate = sampleBufferData->m_sampleRate;
	m_originalData = std::move(sampleBufferData->m_originalData);
	m_originalSampleRate = sampleBufferData->m_originalSampleRate;
}

SampleBuffer::SampleBuffer(int numFrames) : m_data(numFrames) {}

bool SampleBuffer::fileExceedsLimits(const QString& audioFile, bool reportToGui) const
{
	constexpr auto maxFileSize = 300; // In MBs
	constexpr auto maxFileLength = 90; // In minutes
	auto exceedsLimits = QFileInfo{audioFile}.size() > maxFileSize * 1024 * 1024;

	if (!exceedsLimits)
	{
		auto sfInfo = SF_INFO{};
		auto file = QFile{audioFile};
		SNDFILE* sndFile = nullptr;

		if (!file.open(QIODevice::ReadOnly))
		{
			throw std::runtime_error{"Could not open file handle: " + file.errorString().toStdString()};
		}
		else
		{
			sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
			if (sndFile != nullptr)
			{
				exceedsLimits = sfInfo.frames / sfInfo.samplerate > maxFileLength * 60;
			}
			else
			{
				throw std::runtime_error{"Could not open sndfile handle: " + std::string{sf_strerror(sndFile)}};
			}
		}
	}

	if (exceedsLimits && reportToGui)
	{
		const auto title = QObject::tr("Failed to open file");
		const auto message = QObject::tr("Audio files are limited to %1 MB "
				"in size and %2 minutes of playing time").arg(maxFileSize).arg(maxFileLength);

		if (gui::getGUI() != nullptr)
		{
			QMessageBox::information(nullptr, title, message, QMessageBox::Ok);
		}
		else
		{
			std::cerr << message.toUtf8().constData() << '\n';
		}
	}

	return exceedsLimits;
}

void SampleBuffer::decodeSampleSF(const QString& audioFile)
{
	SNDFILE* sndFile = nullptr;
	auto sfInfo = SF_INFO{};

	// Use QFile to handle unicode file names on Windows
	auto file = QFile{audioFile};
	if (!file.open(QIODevice::ReadOnly))
	{
		throw std::runtime_error{"Failed to open sample "
			+ audioFile.toStdString() + ": " + file.errorString().toStdString()};
	}

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0)
	{
		throw std::runtime_error{"Failure opening audio handle: " + std::string{sf_strerror(sndFile)}};
	}

	auto buf = std::vector<sample_t>(sfInfo.channels * sfInfo.frames);
	const auto sfFramesRead = sf_read_float(sndFile, buf.data(), buf.size());

	if (sfFramesRead < sfInfo.channels * sfInfo.frames)
	{
		throw std::runtime_error{"Failure reading audio file: failed to read " + audioFile.toStdString() +
			": " + sf_strerror(sndFile)};
	}

	sf_close(sndFile);
	file.close();

	auto result = std::vector<sampleFrame>(sfInfo.frames);
	for (int i = 0; i < static_cast<int>(result.size()); ++i)
	{
		if (sfInfo.channels == 1)
		{
			// Upmix from mono to stereo
			result[i] = {buf[i], buf[i]};
		}
		else if (sfInfo.channels > 1)
		{
			// TODO: Add support for higher number of channels (i.e., 5.1 channel systems)
			// The current behavior assumes stereo in all cases excluding mono.
			// This may not be the expected behavior, given some audio files with a higher number of channels.
			result[i] = {buf[i * sfInfo.channels], buf[i * sfInfo.channels + 1]};
		}
	}

	m_data = result;
	m_originalData = result;
	m_audioFile = audioFile;
	m_sampleRate = sfInfo.samplerate;
	m_originalSampleRate = sfInfo.samplerate;
}

void SampleBuffer::decodeSampleDS(const QString& audioFile)
{
	auto data = std::unique_ptr<int_sample_t>{};
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto engineRate = Engine::audioEngine()->processingSampleRate();
	const auto frames = ds.GetDSFileSamples(audioFile, dataPtr, DEFAULT_CHANNELS, engineRate);
	data.reset(dataPtr);

	auto result = std::vector<sampleFrame>(frames);
	if (frames > 0 && data != nullptr)
	{
		src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);
	}
	else
	{
		throw std::runtime_error{"Decoding failure: failed to decode DrumSynth file."};
	}

	m_data = result;
	m_originalData = result;
	m_audioFile = audioFile;
	m_sampleRate = engineRate;
	m_originalSampleRate = engineRate;
}

QString SampleBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	auto data = reinterpret_cast<const char*>(this);
	auto byteArray = QByteArray{data, sizeof(*this)};
	return QString::fromUtf8(byteArray.toBase64());
}

void SampleBuffer::resample(sample_rate_t newSampleRate, bool fromOriginal)
{
	const auto srcSampleRate = fromOriginal ? m_originalSampleRate : m_sampleRate;
	const auto dstFrames = static_cast<f_cnt_t>((size() / static_cast<float>(srcSampleRate)) * static_cast<float>(newSampleRate));
	auto dst = std::vector<sampleFrame>(dstFrames);

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	auto srcData = SRC_DATA{};
	srcData.end_of_input = 1;
	srcData.data_in = fromOriginal ? &m_originalData[0][0] : &m_data[0][0];
	srcData.data_out = &dst[0][0];
	srcData.input_frames = fromOriginal ? m_originalData.size() : size();
	srcData.output_frames = dstFrames;
	srcData.src_ratio = static_cast<double>(newSampleRate) / srcSampleRate;

	// TODO: We may want to use the same interpolation mode as the engine
	// in the interest of having consistent expectations
	auto error = src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, DEFAULT_CHANNELS);
	if (error != 0)
	{
		throw std::runtime_error{"SampleBuffer: error while resampling: " + std::string{src_strerror(error)}};
	}

	m_data = std::move(dst);
	m_sampleRate = newSampleRate;
}
} // namespace lmms
