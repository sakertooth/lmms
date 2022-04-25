/*
 * SampleBufferV2.cpp - container-class for SampleBufferV2
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

SampleBufferV2::SampleBufferV2() :
	m_userAntiAliasWaveTable(nullptr),
	m_data(nullptr),
	// TODO C++17 and above: use std::optional instead
	m_strData(),
	m_dataType(AudioDataType::None),
	m_numFrames(0),
	m_startFrame(0),
	m_endFrame(0),
	m_loopStartFrame(0),
	m_loopEndFrame(0),
	m_amplification(1.0f),
	m_reversed(false),
	m_dataIsCached(false),
	m_frequency(DefaultBaseFreq),
	m_sampleRate(Engine::audioEngine()->processingSampleRate())
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(sampleRateChanged()));
	//TODO: update m_data given the current state of member variables
}

SampleBufferV2::SampleBufferV2(const QString& strData, AudioDataType dataType) 
	: SampleBufferV2()
{
	//TODO: implement constructor
}

SampleBufferV2::SampleBufferV2(const sampleFrame *data, const f_cnt_t numFrames)
	: SampleBufferV2()
{
	//TODO: implement constructor
}

SampleBufferV2::SampleBufferV2(const f_cnt_t frames)
	: SampleBufferV2()
{
	//TODO: implement constructor
}

SampleBufferV2::~SampleBufferV2() 
{
	//TODO: C++17 and above: use m_strEmpty.has_value() once m_strEmpty is of type std::optional
	if (m_dataIsCached && !m_strData.isEmpty())
	{
		//Decrement ref for cached sample
	}
}

bool SampleBufferV2::play(sampleFrame *dst, HandleState *state, const fpp_t numFrames, const float freq, const LoopMode loopMode)
{
	//TODO: implement function
}

void SampleBufferV2::visualize(QPainter &painter, const QRect &drawingRectangle, const QRect &clip, f_cnt_t fromFrame, f_cnt_t toFrame) 
{
	//TODO: implement function
}

void SampleBufferV2::visualize(QPainter &painter, const QRect &drawingRectangle, f_cnt_t fromFrame, f_cnt_t toFrame) 
{
	//TODO: implement function
}

void SampleBufferV2::reloadData(bool keepSettings)
{
	//TODO: implement function
}

f_cnt_t SampleBufferV2::numFrames() const 
{
	return m_numFrames;
}

f_cnt_t SampleBufferV2::startFrame() const 
{
	return m_startFrame;
}

f_cnt_t SampleBufferV2::endFrame() const 
{
	return m_endFrame;
}

f_cnt_t SampleBufferV2::loopStartFrame() const 
{
	return m_loopStartFrame;
}

f_cnt_t SampleBufferV2::loopEndFrame() const 
{
	return m_loopEndFrame;
}

float SampleBufferV2::amplification() const 
{
	return m_amplification;
}

bool SampleBufferV2::reversed() const 
{
	return m_reversed;
}

float SampleBufferV2::frequency() const 
{
	return m_frequency;
}

sample_rate_t SampleBufferV2::sampleRate() const 
{
	return m_sampleRate;
}

const sampleFrame* SampleBufferV2::data() const 
{
	return m_data;
}

const std::unique_ptr<OscillatorConstants::waveform_t>& SampleBufferV2::userAntiAliasWaveTable() const
{
	return m_userAntiAliasWaveTable;
}

int SampleBufferV2::calculateSampleLength() const
{
	return double(m_endFrame - m_startFrame) / m_sampleRate * 1000;
}

void SampleBufferV2::setStartFrame(const f_cnt_t start) 
{
	m_startFrame = start;
}

void SampleBufferV2::setEndFrame(const f_cnt_t end) 
{
	m_endFrame = end;
}

void SampleBufferV2::setLoopStartFrame(f_cnt_t start) 
{
	m_loopStartFrame = start;
}

void SampleBufferV2::setLoopEndFrame(f_cnt_t end) 
{
	m_loopEndFrame = end;
}

void SampleBufferV2::setAmplification(float amplification) 
{
	m_amplification = amplification;
}

void SampleBufferV2::setReversed(bool on) 
{
	m_reversed = on;
}

void SampleBufferV2::setFrequency(float freq) 
{
	m_frequency = freq;
}

void SampleBufferV2::setSampleRate(sample_rate_t rate) 
{
	m_sampleRate = rate;
}

void SampleBufferV2::setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd) 
{
	m_startFrame = start;
	m_endFrame = end;
	m_loopStartFrame = loopStart;
	m_loopEndFrame = loopEnd;
}

void SampleBufferV2::resample(const sample_rate_t srcSR, const sample_rate_t dstSR) 
{
	//TODO: implement function
}

void SampleBufferV2::normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings) 
{
	//TODO: implement function
}

sample_t SampleBufferV2::userWaveSample(const float sample) const 
{
	//TODO: implement function
}

void SampleBufferV2::loadFromAudioFile(const QString &audioFile) 
{
	//TODO: implement function
}

void SampleBufferV2::loadFromBase64(const QString &data) 
{
	//TODO: implement function
}

void SampleBufferV2::sampleRateChanged() 
{
	//TODO: implement function
}

void SampleBufferV2::convertIntToFloat(int_sample_t *&ibuf, f_cnt_t frames, int channels) 
{
	//TODO: implement function
}

void SampleBufferV2::directFloatWrite(sample_t *&fbuf, f_cnt_t frames, int channels) 
{
	//TODO: implement function
}

void SampleBufferV2::decodeSampleSF(const QString &fileName, sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate) 
{
	//TODO: implement function
}

#ifdef LMMS_HAVE_OGGVORBIS
	f_cnt_t SampleBufferV2::decodeSampleOGGVorbis(const QString &fileName, int_sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate) 
	{
		//TODO: implement function
	}
#endif

void SampleBufferV2::decodeSampleDS(const QString &fileName, int_sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate) 
{
	//TODO: implement function
}

sampleFrame* SampleBufferV2::getSampleFragment(f_cnt_t index, f_cnt_t frames, LoopMode loopMode, sampleFrame **tmp, bool *backwards, f_cnt_t loopStart, f_cnt_t loopEnd, f_cnt_t end) const 
{
	//TODO: implement function
}

f_cnt_t SampleBufferV2::getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const 
{
	if (index < endf)
	{
		return index;
	}

	return startf + (index - startf) % (endf - startf);
}

f_cnt_t SampleBufferV2::getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const 
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