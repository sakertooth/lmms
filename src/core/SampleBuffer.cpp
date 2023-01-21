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
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>


#include <sndfile.h>

#include "AudioEngine.h"
#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"
#include "endian_handling.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "PathUtil.h"

#include "FileDialog.h"

namespace lmms
{
SampleBuffer::SampleBuffer()
{
	QObject::connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, [this](){ sampleRateChanged(); });
}

SampleBuffer::SampleBuffer(SampleBuffer&& other) :
	m_audioFile(std::exchange(other.m_audioFile, "")),
	m_data(std::move(other.m_data)),
	m_sampleRate(std::exchange(other.m_sampleRate, audioEngineSampleRate())),
	m_userAntiAliasWaveTable(std::move(other.m_userAntiAliasWaveTable)) {}

SampleBuffer& SampleBuffer::operator=(SampleBuffer&& other)
{
	m_audioFile = std::exchange(other.m_audioFile, "");
	m_data = std::move(other.m_data);
	m_sampleRate = std::exchange(other.m_sampleRate, audioEngineSampleRate());
	m_userAntiAliasWaveTable = std::move(other.m_userAntiAliasWaveTable);
	return *this;
}

std::shared_ptr<SampleBuffer> SampleBuffer::createFromAudioFile(const QString& audioFile)
{
	auto buffer = std::make_shared<SampleBuffer>();
	buffer->loadFromAudioFile(audioFile);
	return buffer;
}

std::shared_ptr<SampleBuffer> SampleBuffer::createFromBase64(const QString& base64, sample_rate_t sampleRate)
{
	auto buffer = std::make_shared<SampleBuffer>();
	buffer->loadFromBase64(base64, sampleRate);
	return buffer;
}

SampleBuffer::SampleBuffer(const sampleFrame * data, const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_data = std::vector<sampleFrame>(data, data + frames);
		update();
	}
}

SampleBuffer::SampleBuffer(const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_data = std::vector<sampleFrame>(frames);
		update();
	}
}

void SampleBuffer::sampleRateChanged()
{
	// TODO: Resample original data instead of resampling the same data multiple times
	//		 in order to maintain audio quality.
	resample(Engine::audioEngine()->processingSampleRate());
	update();
}

sample_rate_t SampleBuffer::audioEngineSampleRate()
{
	return Engine::audioEngine()->processingSampleRate();
}

void SampleBuffer::update()
{
	if (frames() > 0)
	{
		Oscillator::generateAntiAliasUserWaveTable(this);
	}
}

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
		const auto title = QObject::tr("Fail to open file");
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

sample_t SampleBuffer::userWaveSample(const float sample) const
{
	const auto frames = m_data.size();
	const auto data = m_data.data();
	const auto frame = sample * frames;

	auto f1 = static_cast<f_cnt_t>(frame) % frames;
	if (f1 < 0)
	{
		f1 += frames;
	}

	return linearInterpolate(data[f1][0], data[(f1 + 1) % frames][0], fraction(frame));
}

void SampleBuffer::decodeSampleSF(const QString& fileName)
{
	SNDFILE* sndFile = nullptr;
	auto sfInfo = SF_INFO{};

	// Use QFile to handle unicode file names on Windows
	auto file = QFile{fileName};
	if (!file.open(QIODevice::ReadOnly))
	{
		throw std::runtime_error{"Failed to open sample "
			+ fileName.toStdString() + ": " + file.errorString().toStdString()};
	}

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0)
	{
		throw std::runtime_error{"Failed to open sndfile handle: " + std::string{sf_strerror(sndFile)}};
	}

	auto buf = std::vector<sample_t>(sfInfo.channels * sfInfo.frames);
	const auto sfFramesRead = sf_read_float(sndFile, buf.data(), buf.size());

	if (sfFramesRead < sfInfo.channels * sfInfo.frames)
	{
		throw std::runtime_error{"Failed to read sample " + fileName.toStdString() + ": " + sf_strerror(sndFile)};
	}

	sf_close(sndFile);
	file.close();

	auto result = std::vector<sampleFrame>(sfInfo.frames);
	if (sfInfo.channels == 1)
	{
		// Upmix mono to stereo
		for (int i = 0; i < static_cast<int>(result.size()); ++i)
		{
			result[i] = {buf[i], buf[i]};
		}
	}
	else if (sfInfo.channels > 1)
	{
		// TODO: Add support for higher number of channels (i.e., 5.1 channel systems)
		// The current behavior assumes stereo in all cases excluding mono.
		// This may not be the expected behavior, given some audio files with a higher number of channels.
		std::copy_n(buf.begin(), buf.size(), &result[0][0]);
	}

	m_data = result;
	m_audioFile = fileName;
	m_sampleRate = sfInfo.samplerate;
}

void SampleBuffer::decodeSampleDS(const QString& fileName)
{
	auto data = std::unique_ptr<int_sample_t>{};
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto frames = ds.GetDSFileSamples(fileName, dataPtr, DEFAULT_CHANNELS, audioEngineSampleRate());
	data.reset(dataPtr);

	auto result = std::vector<sampleFrame>(frames);
	if (frames > 0 && data != nullptr)
	{
		src_short_to_float_array(data.get(), &result[0][0], frames * DEFAULT_CHANNELS);
	}
	else
	{
		throw std::runtime_error{"SampleBuffer::decodeSampleDS: Failed to decode DrumSynth file."};
	}

	m_data = result;
	m_audioFile = fileName;
	m_sampleRate = audioEngineSampleRate();
}

QString SampleBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	auto data = reinterpret_cast<const char*>(m_data.data());
	auto byteArray = QByteArray{data, static_cast<int>(m_data.size() * sizeof(sampleFrame))};
	return QString::fromUtf8(byteArray.toBase64());
}

void SampleBuffer::resample(sample_rate_t newSampleRate)
{
	const auto dstFrames = static_cast<f_cnt_t>((frames() / static_cast<float>(m_sampleRate)) * static_cast<float>(newSampleRate));
	auto dst = std::vector<sampleFrame>(dstFrames);

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	auto srcData = SRC_DATA{};
	srcData.end_of_input = 1;
	srcData.data_in = &m_data[0][0];
	srcData.data_out = &dst[0][0];
	srcData.input_frames = frames();
	srcData.output_frames = dstFrames;
	srcData.src_ratio = static_cast<double>(newSampleRate) / m_sampleRate;

	auto error = src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, DEFAULT_CHANNELS);
	if (error != 0)
	{
		std::cout << "SampleBuffer: error while resampling:" << src_strerror(error) << '\n';
	}

	m_data = std::move(dst);
	m_sampleRate = newSampleRate;
}

void SampleBuffer::loadFromAudioFile(const QString& audioFile)
{
	if (audioFile.isEmpty()) { return; }

	Engine::audioEngine()->requestChangeInModel();
	const auto lockGuard = std::unique_lock{m_mutex};

	try
	{
		const auto file = PathUtil::toAbsolute(PathUtil::toShortestRelative(audioFile));
		if (QFileInfo{file}.suffix() == "ds")
		{
			// Note: DrumSynth files aren't checked for file limits since we are using sndfile to detect them.
			// In the future, checking for limits may become unnecessary anyways, so this seems fine for now.
			decodeSampleDS(file);
		}
		else if (!fileExceedsLimits(file))
		{
			decodeSampleSF(file);
		}

		if (m_sampleRate != audioEngineSampleRate())
		{
			resample(audioEngineSampleRate());
		}
	}
	catch (std::runtime_error& error)
	{
		if (gui::getGUI() != nullptr)
		{
			QMessageBox::information(nullptr,
				QObject::tr("File load error"),
				QObject::tr("An error occurred while loading %1").arg(audioFile), QMessageBox::Ok);
		}

		std::cerr << "Could not load audio file: " << error.what() << '\n';
	}

	Engine::audioEngine()->doneChangeInModel();
}

void SampleBuffer::loadFromBase64(const QString& data, sample_rate_t sampleRate)
{
	if (data.isEmpty()) { return; }

	Engine::audioEngine()->requestChangeInModel();
	const auto lockGuard = std::unique_lock{m_mutex};

	// TODO: Replace with non-Qt equivalent
	const auto base64Data = data.toUtf8().toBase64();
	const auto sampleFrameData = reinterpret_cast<const sampleFrame*>(base64Data.constData());
	const auto numFrames = base64Data.size() / sizeof(sampleFrame);

	m_data = std::vector<sampleFrame>(sampleFrameData, sampleFrameData + numFrames);
	m_sampleRate = sampleRate;

	if (sampleRate != audioEngineSampleRate())
	{
		resample(audioEngineSampleRate());
	}

	Engine::audioEngine()->doneChangeInModel();
}

} // namespace lmms
