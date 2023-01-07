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
	m_playMarkers = orig.m_playMarkers;
	m_amplification = orig.m_amplification;
	m_reversed = orig.m_reversed;
	m_frequency = orig.m_frequency;
	m_sampleRate = orig.m_sampleRate;
}




void swap(SampleBuffer& first, SampleBuffer& second) noexcept
{
	using std::swap;

	if (&first == &second) { return; }
	const auto firstLockGuard = std::unique_lock{first.m_mutex};
	const auto secondLockGuard = std::unique_lock{second.m_mutex};

	first.m_audioFile.swap(second.m_audioFile);
	swap(first.m_data, second.m_data);
	swap(first.m_playMarkers, second.m_playMarkers);
	swap(first.m_amplification, second.m_amplification);
	swap(first.m_frequency, second.m_frequency);
	swap(first.m_reversed, second.m_reversed);
	swap(first.m_sampleRate, second.m_sampleRate);
}




SampleBuffer& SampleBuffer::operator=(SampleBuffer that)
{
	swap(*this, that);
	return *this;
}

void SampleBuffer::sampleRateChanged()
{
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

	emit sampleUpdated();
}


void SampleBuffer::convertIntToFloat(
	int_sample_t * & ibuf,
	f_cnt_t frames,
	int channels
)
{
	// following code transforms int-samples into float-samples and does amplifying & reversing
	const float fac = 1 / OUTPUT_SAMPLE_MULTIPLIER;
	m_data = std::vector<sampleFrame>(frames);
	const int ch = (channels > 1) ? 1 : 0;

	// if reversing is on, we also reverse when scaling
	bool isReversed = m_reversed;
	int idx = isReversed ? (frames - 1) * channels : 0;
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		m_data[frame][0] = ibuf[idx+0] * fac;
		m_data[frame][1] = ibuf[idx+ch] * fac;
		idx += isReversed ? -channels : channels;
	}

	delete[] ibuf;
}

void SampleBuffer::directFloatWrite(
	sample_t * & fbuf,
	f_cnt_t frames,
	int channels
)
{

	m_data = std::vector<sampleFrame>(frames);
	const int ch = (channels > 1) ? 1 : 0;

	// if reversing is on, we also reverse when scaling
	bool isReversed = m_reversed;
	int idx = isReversed ? (frames - 1) * channels : 0;
	for (f_cnt_t frame = 0; frame < frames; ++frame)
	{
		m_data[frame][0] = fbuf[idx+0];
		m_data[frame][1] = fbuf[idx+ch];
		idx += isReversed ? -channels : channels;
	}

	delete[] fbuf;
}

bool SampleBuffer::fileExceedsLimits(const QString& audioFile, bool reportToGui)
{
	if (audioFile.isEmpty()) { return true; }

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
			throw std::runtime_error{"SampleBuffer::fileExceedsLimit: Could not open file handle: " +
				file.errorString().toStdString()};
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
				throw std::runtime_error{"SampleBuffer::fileExceedsLimit: Could not open sndfile handle: " +
					std::string{sf_strerror(sndFile)}};
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
		auto& [startFrame, endFrame, loopStartFrame, loopEndFrame] = m_playMarkers;

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
	const float frame = sample * frames;
	f_cnt_t f1 = static_cast<f_cnt_t>(frame) % frames;
	if (f1 < 0)
	{
		f1 += frames;
	}
	return linearInterpolate(data[f1][0], data[(f1 + 1) % frames][0], fraction(frame));
}

f_cnt_t SampleBuffer::decodeSampleSF(
	QString fileName,
	sample_t * & buf,
	ch_cnt_t & channels,
	sample_rate_t & samplerate
)
{
	SNDFILE * sndFile;
	SF_INFO sfInfo;
	sfInfo.format = 0;
	f_cnt_t frames = 0;
	sf_count_t sfFramesRead;


	// Use QFile to handle unicode file names on Windows
	QFile f(fileName);
	if (f.open(QIODevice::ReadOnly) && (sndFile = sf_open_fd(f.handle(), SFM_READ, &sfInfo, false)))
	{
		frames = sfInfo.frames;

		buf = new sample_t[sfInfo.channels * frames];
		sfFramesRead = sf_read_float(sndFile, buf, sfInfo.channels * frames);

		if (sfFramesRead < sfInfo.channels * frames)
		{
#ifdef DEBUG_LMMS
			qDebug("SampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s", fileName, sf_strerror(nullptr));
#endif
		}
		channels = sfInfo.channels;
		samplerate = sfInfo.samplerate;

		sf_close(sndFile);
	}
	else
	{
#ifdef DEBUG_LMMS
		qDebug("SampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s", fileName, sf_strerror(nullptr));
#endif
	}
	f.close();

	//write down either directly or convert i->f depending on file type

	if (frames > 0 && buf != nullptr)
	{
		directFloatWrite(buf, frames, channels);
	}

	return frames;
}

f_cnt_t SampleBuffer::decodeSampleDS(
	QString fileName,
	int_sample_t * & buf,
	ch_cnt_t & channels,
	sample_rate_t & samplerate
)
{
	DrumSynth ds;
	f_cnt_t frames = ds.GetDSFileSamples(fileName, buf, channels, samplerate);

	if (frames > 0 && buf != nullptr)
	{
		convertIntToFloat(buf, frames, channels);
	}

	return frames;

}




bool SampleBuffer::play(
	sampleFrame * ab,
	Sample * state,
	const fpp_t frames,
	const float freq,
	const LoopMode loopMode
)
{
	auto& [startFrame, endFrame, loopStartFrame, loopEndFrame] = m_playMarkers;

	if (endFrame == 0 || frames == 0)
	{
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool isBackwards = state->isBackwards();

	// The SampleBuffer can play a given sample with increased or decreased pitch. However, only
	// samples that contain a tone that matches the default base note frequency of 440 Hz will
	// produce the exact requested pitch in [Hz].
	const double freqFactor = (double) freq / (double) m_frequency *
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
		// Advance
		switch (loopMode)
		{
			case LoopMode::LoopOff:
				playFrame += srcData.input_frames_used;
				break;
			case LoopMode::LoopOn:
				playFrame += srcData.input_frames_used;
				playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			case LoopMode::LoopPingPong:
			{
				f_cnt_t left = srcData.input_frames_used;
				if (state->isBackwards())
				{
					playFrame -= srcData.input_frames_used;
					if (playFrame < loopStartFrame)
					{
						left -= (loopStartFrame - playFrame);
						playFrame = loopStartFrame;
					}
					else left = 0;
				}
				playFrame += left;
				playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			}
		}
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		const auto sampleFragment = getSampleFragment(playFrame, frames, loopMode, &isBackwards,
				loopStartFrame, loopEndFrame, endFrame);
		std::copy_n(sampleFragment.begin(), frames, ab);

		// Advance
		switch (loopMode)
		{
			case LoopMode::LoopOff:
				playFrame += frames;
				break;
			case LoopMode::LoopOn:
				playFrame += frames;
				playFrame = getLoopedIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			case LoopMode::LoopPingPong:
			{
				f_cnt_t left = frames;
				if (state->isBackwards())
				{
					playFrame -= frames;
					if (playFrame < loopStartFrame)
					{
						left -= (loopStartFrame - playFrame);
						playFrame = loopStartFrame;
					}
					else left = 0;
				}
				playFrame += left;
				playFrame = getPingPongIndex(playFrame, loopStartFrame, loopEndFrame);
				break;
			}
		}
	}

	state->setBackwards(isBackwards);
	state->setFrameIndex(playFrame);

	for (fpp_t i = 0; i < frames; ++i)
	{
		ab[i][0] *= m_amplification;
		ab[i][1] *= m_amplification;
	}

	return true;
}




std::vector<sampleFrame> SampleBuffer::getSampleFragment(
	f_cnt_t index,
	f_cnt_t frames,
	LoopMode loopMode,
	bool * backwards,
	f_cnt_t loopStart,
	f_cnt_t loopEnd,
	f_cnt_t end
) const
{
	auto out = std::vector<sampleFrame>(frames);
	if (loopMode == LoopMode::LoopOff)
	{
		if (index + frames <= end)
		{
			std::copy_n(m_data.begin() + index, frames, out.begin());
			return out;
		}
	}
	else if (loopMode == LoopMode::LoopOn)
	{
		if (index + frames <= loopEnd)
		{
			std::copy_n(m_data.begin() + index, frames, out.begin());
			return out;
		}
	}
	else
	{
		if (!*backwards && index + frames < loopEnd)
		{
			std::copy_n(m_data.begin() + index, frames, out.begin());
			return out;
		}
	}


	if (loopMode == LoopMode::LoopOff)
	{
		const auto available = end - index;
		std::copy_n(m_data.begin() + index, available, out.begin());
		std::fill_n(out.begin() + available, frames - available, sampleFrame{});
	}
	else if (loopMode == LoopMode::LoopOn)
	{
		f_cnt_t copied = std::min(frames, loopEnd - index);
		std::copy_n(m_data.begin() + index, copied, out.begin());
		f_cnt_t loopFrames = loopEnd - loopStart;
		while (copied < frames)
		{
			f_cnt_t todo = std::min(frames - copied, loopFrames);
			std::copy_n(m_data.begin() + loopStart, todo, out.begin() + copied);
			copied += todo;
		}
	}
	else
	{
		f_cnt_t pos = index;
		bool currentBackwards = pos < loopStart
			? false
			: *backwards;
		f_cnt_t copied = 0;


		if (currentBackwards)
		{
			copied = std::min(frames, pos - loopStart);
			for (int i = 0; i < copied; i++)
			{
				out[i][0] = m_data[pos - i][0];
				out[i][1] = m_data[pos - i][1];
			}
			pos -= copied;
			if (pos == loopStart) { currentBackwards = false; }
		}
		else
		{
			copied = std::min(frames, loopEnd - pos);
			std::copy_n(m_data.begin() + pos, copied, out.begin());
			pos += copied;
			if (pos == loopEnd) { currentBackwards = true; }
		}

		while (copied < frames)
		{
			if (currentBackwards)
			{
				f_cnt_t todo = std::min(frames - copied, pos - loopStart);
				for (int i = 0; i < todo; i++)
				{
					out[copied + i][0] = m_data[pos - i][0];
					out[copied + i][1] = m_data[pos - i][1];
				}
				pos -= todo;
				copied += todo;
				if (pos <= loopStart) { currentBackwards = false; }
			}
			else
			{
				f_cnt_t todo = std::min(frames - copied, loopEnd - pos);
				std::copy_n(m_data.begin() + pos, todo, out.begin() + copied);
				pos += todo;
				copied += todo;
				if (pos >= loopEnd) { currentBackwards = true; }
			}
		}
		*backwards = currentBackwards;
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
	const QRect & clip,
	f_cnt_t fromFrame,
	f_cnt_t toFrame
)
{
	const auto numFrames = frames();
	if (numFrames == 0) { return; }

	const bool focusOnRange = toFrame <= numFrames && 0 <= fromFrame && fromFrame < toFrame;
	//TODO: If the clip QRect is not being used we should remove it
	//p.setClipRect(clip);
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
		auto py = ySpace * m_amplification;
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

void SampleBuffer::visualize(QPainter & p, const QRect & dr, f_cnt_t fromFrame, f_cnt_t toFrame)
{
	visualize(p, dr, dr, fromFrame, toFrame);
}

const QString& SampleBuffer::audioFile() const
{
	return m_audioFile;
}

f_cnt_t SampleBuffer::startFrame() const
{
	return m_playMarkers.startFrame;
}

f_cnt_t SampleBuffer::endFrame() const
{
	return m_playMarkers.endFrame;
}

f_cnt_t SampleBuffer::loopStartFrame() const
{
	return m_playMarkers.loopStartFrame;
}

f_cnt_t SampleBuffer::loopEndFrame() const
{
	return m_playMarkers.loopEndFrame;
}

void SampleBuffer::setLoopStartFrame(f_cnt_t start)
{
	m_playMarkers.loopStartFrame = start;
}

void SampleBuffer::setLoopEndFrame(f_cnt_t end)
{
	m_playMarkers.loopEndFrame = end;
}

void SampleBuffer::setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd)
{
	m_playMarkers = {start, end, loopStart, loopEnd};
}

std::shared_mutex& SampleBuffer::mutex() const
{
	return m_mutex;
}

f_cnt_t SampleBuffer::frames() const
{
	return static_cast<f_cnt_t>(m_data.size());
}

float SampleBuffer::amplification() const
{
	return m_amplification;
}

bool SampleBuffer::reversed() const
{
	return m_reversed;
}

float SampleBuffer::frequency() const
{
	return m_frequency;
}

sample_rate_t SampleBuffer::sampleRate() const
{
	return m_sampleRate;
}

const std::unique_ptr<OscillatorConstants::waveform_t>& SampleBuffer::userAntiAliasWaveTable() const
{
	return m_userAntiAliasWaveTable;
}

int SampleBuffer::sampleLength() const
{
	return static_cast<double>(m_playMarkers.endFrame - m_playMarkers.startFrame) / m_sampleRate * 1000;
}

void SampleBuffer::setFrequency(float freq)
{
	m_frequency = freq;
}

void SampleBuffer::setSampleRate(sample_rate_t rate)
{
	m_sampleRate = rate;
}

const sampleFrame* SampleBuffer::data() const
{
	return m_data.data();
}

QString SampleBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	auto data = reinterpret_cast<const char*>(m_data.data());
	auto byteArray = QByteArray{data, frames()};
	return QString::fromUtf8(byteArray.toBase64());
}

const std::array<f_cnt_t, 5>& SampleBuffer::interpolationMargins()
{
	return s_interpolationMargins;
}

std::vector<sampleFrame> SampleBuffer::resample(const sample_rate_t srcSR, const sample_rate_t dstSR )
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

	m_audioFile = PathUtil::toShortestRelative(audioFile);
	const auto file = PathUtil::toAbsolute(m_audioFile);

	auto bail = [&]()
	{
		m_data = std::vector<sampleFrame>(1);
		setAllPointFrames(0, 1, 0, 1);
	};

	auto audioFileTooLarge = false;
	try
	{
		audioFileTooLarge = fileExceedsLimits(file);
	}
	catch (const std::runtime_error& error)
	{
		std::cout << error.what() << '\n';
		bail();
	}

	sample_rate_t samplerate = audioEngineSampleRate();
	if (!audioFileTooLarge)
	{
		int_sample_t * buf = nullptr;
		sample_t * fbuf = nullptr;
		ch_cnt_t channels = DEFAULT_CHANNELS;

		if (QFileInfo{file}.suffix() == "ds")
		{
			decodeSampleDS(file, buf, channels, samplerate);
		}
		else
		{
			decodeSampleSF(file, fbuf, channels, samplerate);
		}
	}

	if (frames() == 0 || audioFileTooLarge)  // if still no frames, bail
	{
		// sample couldn't be decoded, create buffer containing
		// one sample-frame
		bail();
	}
	else // otherwise normalize sample rate
	{
		normalizeSampleRate(samplerate, keepSettings);
		update();
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




void SampleBuffer::setStartFrame(const f_cnt_t s)
{
	m_playMarkers.startFrame = s;
}




void SampleBuffer::setEndFrame(const f_cnt_t e)
{
	m_playMarkers.endFrame = e;
}




void SampleBuffer::setAmplification(float a)
{
	m_amplification = a;
	emit sampleUpdated();
}




void SampleBuffer::setReversed(bool on)
{
	Engine::audioEngine()->requestChangeInModel();
	const auto lockGuard = std::unique_lock{m_mutex};

	if (m_reversed != on) { std::reverse(m_data.begin(), m_data.end()); }
	m_reversed = on;

	Engine::audioEngine()->doneChangeInModel();
	emit sampleUpdated();
}

} // namespace lmms
