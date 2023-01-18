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

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
static auto s_interpolationMargins = std::array<f_cnt_t, 5>{64, 64, 64, 4, 4};

SampleBuffer::SampleBuffer()
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
}

SampleBuffer::SampleBuffer(const QString& audioFile, bool isBase64Data)
	: SampleBuffer()
{
	if (isBase64Data)
	{
		loadFromBase64(audioFile);
	}
	else
	{
		loadFromAudioFile(audioFile);
	}
}

SampleBuffer::SampleBuffer(const sampleFrame * data, const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_data = std::vector<sampleFrame>(data, data + frames);
		setAllPointFrames(0, frames, 0, frames);
		update();
	}
}

SampleBuffer::SampleBuffer(const f_cnt_t frames)
	: SampleBuffer()
{
	if (frames > 0)
	{
		m_data = std::vector<sampleFrame>(frames);
		setAllPointFrames(0, frames, 0, frames);
		update();
	}
}

SampleBuffer::SampleBuffer(const SampleBuffer& orig)
{
	const auto lockGuard = std::shared_lock{orig.m_mutex};
	m_audioFile = orig.m_audioFile;
	m_data = orig.m_data;
	m_sample.m_playMarkers = orig.m_sample.m_playMarkers;
	m_sample.m_amplification = orig.m_sample.m_amplification;
	m_reversed = orig.m_reversed;
	m_sample.m_frequency = orig.m_sample.m_frequency;
	m_sampleRate = orig.m_sampleRate;
}

void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;

	if (&first == &second) { return; }
	const auto lockGuard = std::scoped_lock{first.m_mutex, second.m_mutex};

	first.m_audioFile.swap(second.m_audioFile);
	swap(first.m_data, second.m_data);
	swap(first.m_reversed, second.m_reversed);
	swap(first.m_sampleRate, second.m_sampleRate);

	// TODO: Currently inaccessible due to the move within Sample
	// swap(first.m_sample.m_playMarkers, second.m_sample.m_playMarkers);
	// swap(first.m_sample.m_amplification, second.m_sample.m_amplification);
	// swap(first.m_sample.m_frequency, second.m_sample.m_frequency);
}

SampleBuffer& SampleBuffer::operator=(SampleBuffer that)
{
	swap(*this, that);
	return *this;
}

void SampleBuffer::sampleRateChanged()
{
	normalizeSampleRate(m_sampleRate, true);
	update();
}

sample_rate_t SampleBuffer::audioEngineSampleRate() const
{
	return Engine::audioEngine()->processingSampleRate();
}

void SampleBuffer::update()
{
	if (frames() > 0)
	{
		Oscillator::generateAntiAliasUserWaveTable(this);
	}

	emit sampleUpdated();
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
		const auto title = tr("Fail to open file");
		const auto message = tr("Audio files are limited to %1 MB "
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

void SampleBuffer::normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings)
{
	const sample_rate_t oldRate = m_sampleRate;
	// do samplerate-conversion to our default-samplerate
	if (srcSR != audioEngineSampleRate())
	{
		resample(audioEngineSampleRate());
	}

	if (keepSettings == false)
	{
		// update frame-variables
		setAllPointFrames(0, frames(), 0, frames());
	}
	else if (oldRate != audioEngineSampleRate())
	{
		auto oldRateToNewRateRatio = static_cast<float>(audioEngineSampleRate()) / oldRate;
		const auto numFrames = frames();
		auto& [startFrame, endFrame, loopStartFrame, loopEndFrame] = m_sample.m_playMarkers;

		startFrame = std::clamp(static_cast<f_cnt_t>(startFrame * oldRateToNewRateRatio), 0, numFrames);
		endFrame = std::clamp(static_cast<f_cnt_t>(endFrame * oldRateToNewRateRatio), startFrame, numFrames);
		loopStartFrame = std::clamp(static_cast<f_cnt_t>(loopStartFrame * oldRateToNewRateRatio), 0, numFrames);
		loopEndFrame = std::clamp(static_cast<f_cnt_t>(loopEndFrame * oldRateToNewRateRatio), loopStartFrame, numFrames);
		m_sampleRate = audioEngineSampleRate();
	}
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

	if (m_reversed)
	{
		std::reverse(result.begin(), result.end());
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
		if (m_reversed)
		{
			std::reverse(result.begin(), result.end());
		}
	}
	else
	{
		throw std::runtime_error{"SampleBuffer::decodeSampleDS: Failed to decode DrumSynth file."};
	}

	m_data = result;
	m_audioFile = fileName;
	m_sampleRate = audioEngineSampleRate();
}

bool SampleBuffer::play(
	sampleFrame * ab,
	Sample::PlaybackState * state,
	const fpp_t frames,
	const float freq,
	const LoopMode loopMode
)
{
	return m_sample.play(this, ab, state, frames, freq,
		static_cast<Sample::LoopMode>(static_cast<int>(loopMode)));
}

void SampleBuffer::visualize(
	QPainter & p,
	const QRect & dr,
	f_cnt_t fromFrame,
	f_cnt_t toFrame
)
{
	m_sample.visualize(this, p, dr, fromFrame, toFrame);
}

int SampleBuffer::sampleLength() const
{
	return static_cast<double>(m_sample.m_playMarkers.endFrame - m_sample.m_playMarkers.startFrame) / m_sampleRate * 1000;
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

void SampleBuffer::loadFromAudioFile(const QString& audioFile, bool keepSettings)
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
			setAllPointFrames(0, frames(), 0, frames());
			update();
		}
		else if (!fileExceedsLimits(file))
		{
			decodeSampleSF(file);
			normalizeSampleRate(m_sampleRate, keepSettings);
			update();
		}
	}
	catch (std::runtime_error& error)
	{
		if (gui::getGUI() != nullptr)
		{
			QMessageBox::information(nullptr,
				tr("File load error"),
				tr("An error occurred while loading %1").arg(audioFile), QMessageBox::Ok);
		}

		std::cerr << "Could not load audio file: " << error.what() << '\n';
	}

	Engine::audioEngine()->doneChangeInModel();
}

void SampleBuffer::loadFromBase64(const QString& data, bool keepSettings)
{
	if (data.isEmpty()) { return; }

	Engine::audioEngine()->requestChangeInModel();
	const auto lockGuard = std::unique_lock{m_mutex};

	// TODO: Replace with non-Qt equivalent
	const auto base64Data = data.toUtf8().toBase64();
	const auto sampleFrameData = reinterpret_cast<const sampleFrame*>(base64Data.constData());
	const auto numFrames = base64Data.size() / sizeof(sampleFrame);

	m_data = std::vector<sampleFrame>(sampleFrameData, sampleFrameData + numFrames);

	if (!keepSettings)
	{
		setAllPointFrames(0, numFrames, 0, numFrames);
	}

	Engine::audioEngine()->doneChangeInModel();
	update();
}

void SampleBuffer::setAmplification(float a)
{
	m_sample.m_amplification = a;
	emit sampleUpdated();
}

void SampleBuffer::setReversed(bool on)
{
	// TODO: Locking while reversing creates noticeable pauses.
	// Change later so that this operation happens in real-time.
	Engine::audioEngine()->requestChangeInModel();
	const auto lockGuard = std::unique_lock{m_mutex};

	if (m_reversed != on) { std::reverse(m_data.begin(), m_data.end()); }
	m_reversed = on;

	Engine::audioEngine()->doneChangeInModel();
	emit sampleUpdated();
}

} // namespace lmms
