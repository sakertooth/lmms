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
		m_sampleRate = audioEngineSampleRate();
		m_data = resample(srcSR, audioEngineSampleRate());
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

std::pair<std::vector<sampleFrame>, sample_rate_t> SampleBuffer::decodeSampleSF(const QString& fileName) const
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

	return {result, sfInfo.samplerate};
}

std::vector<sampleFrame> SampleBuffer::decodeSampleDS(const QString& fileName) const
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

	return result;
}

bool SampleBuffer::play(
	sampleFrame * ab,
	Sample::PlaybackState * state,
	const fpp_t frames,
	const float freq,
	const LoopMode loopMode
)
{
	const auto& [startFrame, endFrame, loopStartFrame, loopEndFrame] = m_sample.m_playMarkers;

	if (endFrame == 0 || frames == 0)
	{
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool isBackwards = state->isBackwards();

	// The SampleBuffer can play a given sample with increased or decreased pitch. However, only
	// samples that contain a tone that matches the default base note frequency of 440 Hz will
	// produce the exact requested pitch in [Hz].
	const double freqFactor = (double) freq / (double) m_sample.m_frequency *
		m_sampleRate / Engine::audioEngine()->processingSampleRate();

	// calculate how many frames we have in requested pitch
	const auto totalFramesForCurrentPitch = static_cast<f_cnt_t>((endFrame - startFrame) / freqFactor);

	if (totalFramesForCurrentPitch == 0)
	{
		return false;
	}


	// this holds the index of the first frame to play
	f_cnt_t playFrame = std::max(state->m_frameIndex, startFrame);

	if (loopMode == LoopMode::LoopOff)
	{
		if (playFrame >= endFrame || (endFrame - playFrame) / freqFactor == 0)
		{
			// the sample is done being played
			return false;
		}
	}
	else if (loopMode == LoopMode::LoopOn)
	{
		playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
	}
	else
	{
		playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
	}

	f_cnt_t fragmentSize = (f_cnt_t)(frames * freqFactor) + s_interpolationMargins[state->interpolationMode()];

	// check whether we have to change pitch...
	if (freqFactor != 1.0 || state->m_varyingPitch)
	{
		SRC_DATA srcData;
		// Generate output
		const auto sampleFragment = getSampleFragment(playFrame, fragmentSize, loopMode, &isBackwards,
			loopStartFrame, loopEndFrame, endFrame);
		srcData.data_in = &sampleFragment[0][0];
		srcData.data_out = ab->data();
		srcData.input_frames = fragmentSize;
		srcData.output_frames = frames;
		srcData.src_ratio = 1.0 / freqFactor;
		srcData.end_of_input = 0;
		int error = src_process(state->m_resamplingData, &srcData);
		if (error)
		{
			std::cerr << "SampleBuffer: error while resampling: " << src_strerror(error) << '\n';
		}

		if (srcData.output_frames_gen > frames)
		{
			std::cerr << "SampleBuffer: not enough frames: " << srcData.output_frames_gen << " / " << frames << '\n';
		}

		playFrame = advance(playFrame, srcData.input_frames_used, loopMode, state);
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		const auto sampleFragment = getSampleFragment(playFrame, frames, loopMode, &isBackwards,
				loopStartFrame, loopEndFrame, endFrame);
		std::copy_n(sampleFragment.begin(), frames, ab);
		playFrame = advance(playFrame, frames, loopMode, state);
	}

	state->setBackwards(isBackwards);
	state->setFrameIndex(playFrame);

	for (fpp_t i = 0; i < frames; ++i)
	{
		ab[i][0] *= m_sample.m_amplification;
		ab[i][1] *= m_sample.m_amplification;
	}

	return true;
}

f_cnt_t SampleBuffer::advance(f_cnt_t playFrame, f_cnt_t frames,  LoopMode loopMode, Sample::PlaybackState* state)
{
	switch (loopMode)
	{
		case LoopMode::LoopOff:
			return playFrame + frames;
		case LoopMode::LoopOn:
			return frames + getLoopedIndex(playFrame, m_sample.m_playMarkers.loopStartFrame, m_sample.m_playMarkers.loopEndFrame);
		case LoopMode::LoopPingPong:
		{
			f_cnt_t left = frames;
			if (state->isBackwards())
			{
				playFrame -= frames;
				if (playFrame < m_sample.m_playMarkers.loopStartFrame)
				{
					left -= m_sample.m_playMarkers.loopStartFrame - playFrame;
					playFrame = m_sample.m_playMarkers.loopStartFrame;
				}
				else
				{
					left = 0;
				}
			}

			return left + getPingPongIndex(playFrame, m_sample.m_playMarkers.loopStartFrame, m_sample.m_playMarkers.loopEndFrame);
		}
		default:
			return playFrame;
	}
}

std::vector<sampleFrame> SampleBuffer::getSampleFragment(
	f_cnt_t currentFrame,
	f_cnt_t numFramesRequested,
	LoopMode loopMode,
	bool * backwards,
	f_cnt_t loopStart,
	f_cnt_t loopEnd,
	f_cnt_t endFrame
) const
{
	auto out = std::vector<sampleFrame>(numFramesRequested);
	if (loopMode == LoopMode::LoopOff)
	{
		// Specify the number of frames to copy, starting at index
		// If there are not enough frames to copy, copy the remaining frames
		// (endFrame - currentFrame)
		const auto numFramesToCopy = std::min(numFramesRequested, endFrame - currentFrame);
		assert(numFramesToCopy >= 0);
		std::copy_n(m_data.begin() + currentFrame, numFramesToCopy, out.begin());
	}
	else if (loopMode == LoopMode::LoopOn || loopMode == LoopMode::LoopPingPong)
	{
		// Will be used to track the current play position
		// and how many frames we copied while looping
		auto playPosition = currentFrame;
		auto numFramesCopied = 0;

		// Move playPosition to loopStart if playPosition has past or is currently at loopEnd
		if (playPosition >= loopEnd) { playPosition = loopStart; }

		while (numFramesCopied != numFramesRequested)
		{
			// If we do not have enough frames to copy, then one of these conditions must be true:
			// 1. playPosition + numFramesRequested > loopEnd for LoopOn mode and LoopPingPong mode (non-backwards),
			// 2. playPosition - numFramesRequested < loopStart for LoopPingPong mode (backwards),

			// If any of the previous conditions are true, then we should only copy the remaining frames
			// we have in the following ranges:
			// 1. loopEnd - playPosition for LoopOn mode and LoopPingPongMode (non-backwards)
			// 2. playPosition - loopStart for LoopPingPong mode (backwards).

			// Otherwise, we want to copy numFramesRequested frames minus how many frames we already copied

			const auto remainingFrames = (*backwards && loopMode == LoopMode::LoopPingPong) ?
				playPosition - loopStart : loopEnd - playPosition;
			const auto numFramesToCopy = std::min(numFramesRequested - numFramesCopied, remainingFrames);
			assert(numFramesToCopy >= 0);

			if (loopMode == LoopMode::LoopOn || (!*backwards && loopMode == LoopMode::LoopPingPong))
			{
				std::copy_n(m_data.begin() + playPosition, numFramesToCopy, out.begin() + numFramesCopied);
			}
			else if (*backwards && loopMode == LoopMode::LoopPingPong)
			{
				auto distanceFromPlayPosition = std::distance(m_data.begin() + playPosition, m_data.end());
				std::copy_n(m_data.rbegin() + distanceFromPlayPosition, numFramesToCopy, out.begin() + numFramesCopied);
			}

			playPosition += (*backwards && loopMode == LoopMode::LoopPingPong ? -numFramesToCopy : numFramesToCopy);
			numFramesCopied += numFramesToCopy;

			// Ensure that playPosition is in the valid range [0, loopEnd]
			assert(playPosition >= 0 && playPosition <= loopEnd);

			// numFramesCopied can only be less than or equal to numFramesRequested
			assert(numFramesCopied <= numFramesRequested);

			if (playPosition == loopEnd && loopMode == LoopMode::LoopOn)
			{
				playPosition = loopStart;
			}
			else if (playPosition == loopEnd && loopMode == LoopMode::LoopPingPong)
			{
				*backwards = true;
			}
			else if (playPosition == loopStart && *backwards && loopMode == LoopMode::LoopPingPong)
			{
				*backwards = false;
			}
		}
	}
	
	return out;
}

f_cnt_t SampleBuffer::getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf)
	{
		return index;
	}
	return startf + (index - startf) % (endf - startf);
}


f_cnt_t SampleBuffer::getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const
{
	if (index < endf)
	{
		return index;
	}
	const f_cnt_t loopLen = endf - startf;
	const f_cnt_t loopPos = (index - endf) % (loopLen * 2);

	return (loopPos < loopLen)
		? endf - loopPos
		: startf + (loopPos - loopLen);
}

/* @brief Draws a sample buffer on the QRect given in the range [fromFrame, toFrame)
 * @param QPainter p: Painter object for the painting operations
 * @param QRect dr: QRect where the buffer will be drawn in
 * @param QRect clip: QRect used for clipping
 * @param f_cnt_t fromFrame: First frame of the range
 * @param f_cnt_t toFrame: Last frame of the range non-inclusive
 */
void SampleBuffer::visualize(
	QPainter & p,
	const QRect & dr,
	f_cnt_t fromFrame,
	f_cnt_t toFrame
)
{
	const auto numFrames = frames();
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
	const int totalPoints = nbFrames > w
		? w
		: nbFrames;
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
	const int lastVisibleFrame = focusOnRange
		? fromFrame + visibleFrames - 1
		: visibleFrames - 1;

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
				auto curData = m_data[static_cast<int>(frame) + i][j];

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
		auto x = nbFrames >= w
			? xb + curPixel
			: xb + ((static_cast<double>(curPixel) / nbFrames) * w);
		// Partial Y calculation
		auto py = ySpace * m_sample.m_amplification;
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

const std::array<f_cnt_t, 5>& SampleBuffer::interpolationMargins()
{
	return s_interpolationMargins;
}

std::vector<sampleFrame> SampleBuffer::resample(const sample_rate_t srcSR, const sample_rate_t dstSR)
{
	const auto dstFrames = static_cast<f_cnt_t>((frames() / static_cast<float>(srcSR)) * static_cast<float>(dstSR));
	auto dst = std::vector<sampleFrame>(dstFrames);

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	auto srcData = SRC_DATA{};
	srcData.end_of_input = 1;
	srcData.data_in = &m_data[0][0];
	srcData.data_out = &dst[0][0];
	srcData.input_frames = frames();
	srcData.output_frames = dstFrames;
	srcData.src_ratio = static_cast<double>(dstSR) / srcSR;

	auto error = src_simple(&srcData, SRC_SINC_MEDIUM_QUALITY, DEFAULT_CHANNELS);
	if (error != 0)
	{
		std::cout << "SampleBuffer: error while resampling:" << src_strerror(error) << '\n';
	}

	return dst;
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
			m_data = decodeSampleDS(file);
			m_audioFile = file;
			setAllPointFrames(0, frames(), 0, frames());
			update();
		}
		else if (!fileExceedsLimits(file))
		{
			auto [data, sampleRate] = decodeSampleSF(file);
			m_data = std::move(data);
			m_audioFile = file;
			normalizeSampleRate(sampleRate, keepSettings);
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
