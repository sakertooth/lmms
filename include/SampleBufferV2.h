/*
 * SampleBufferV2.h - container-class for SampleBufferV2
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
#include <QObject>

#include <samplerate.h>
#include <HandleState.h>

#include "interpolation.h"
#include "lmms_basics.h"
#include "OscillatorConstants.h"
#include "SampleFrames.h"

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
const f_cnt_t MARGIN[] = { 64, 64, 64, 4, 4 };
 
class SampleBuffer : public QObject
{
	Q_OBJECT
public:
	enum class LoopMode
	{
		LoopOff, LoopOn, LoopPingPong
	};

	enum class AudioDataType
	{
		AudioFile, Base64, Frames, None
	}

	SampleBuffer();
	SampleBuffer(const QString &strData, AudioDataType dataType);
	explicit SampleBuffer(const sampleFrame *data, const f_cnt_t numFrames);
	explicit SampleBuffer(const f_cnt_t frames);

	SampleBuffer(const SampleBuffer &other) = delete;
	SampleBuffer &operator=(const SampleBuffer &other) = delete;

	bool play(sampleFrame *dst, HandleState *state, const fpp_t numFrames, const float freq, const LoopMode loopMode = LoopMode::LoopOff);
	void visualize(QPainter &painter, const QRect &drawingRectangle, const QRect &clip, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);
	void visualize(QPainter &painter, const QRect &drawingRectangle, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);

	f_cnt_t startFrame() const;
	f_cnt_t endFrame() const;
	f_cnt_t loopStartFrame() const;
	f_cnt_t loopEndFrame() const;
	float amplification() const;
	bool reversed() const;
	float frequency() const;
	sample_rate_t sampleRate() const;
	std::shared_ptr<SampleFrames> data() const;

	int calculateSampleLength() const;

	void setLoopStartFrame(f_cnt_t start);
	void setLoopEndFrame(f_cnt_t end);
	void setAllPointFrames(f_cnt_t, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd);
	void setFrequency(float freq);
	void setSampleRate(sample_rate_t rate);

	void resample(const sample_rate_t srcSR, const sample_rate_t dstSR);
	void normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings = false);
	sample_t userWaveSample(const float sample) const;

public slots:
	void loadFromAudioFile(const QString &audioFile);
	void loadFromBase64(const QString &data);

	void setStartFrame(const f_cnt_t start);
	void setEndFrame(const f_cnt_t end);
	void setAmplificiation(float a);
	void setReversed(bool on);

	void sampleRateChanged();

private:
	void convertIntToFloat(int_sample_t *&ibuf, f_cnt_t frames, int channels);
	void directFloatWrite(sample_t *&fbuf, f_cnt_t frames, int channels);

	void decodeSampleSF(const QString &fileName, sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate);

#ifdef LMMS_HAVE_OGGVORBIS
	f_cnt_t decodeSampleOGGVorbis(const QString &fileName, int_sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate);
#endif

	void decodeSampleDS(const QString &fileName, int_sample_t *&buf, ch_cnt_t &channels, sample_rate_t &sampleRate);

	sampleFrame *getSampleFragment(f_cnt_t index, f_cnt_t frames, LoopMode loopMode, sampleFrame **tmp, bool *backwards, f_cnt_t loopStart, f_cnt_t loopEnd, f_cnt_t end) const;

	f_cnt_t getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;
	f_cnt_t getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;

private:
	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable;
	std::shared_ptr<SampleFrames> m_data;
	mutable QReadWriteLock m_varLock;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loopStartFrame;
	f_cnt_t m_loopEndFrame;
	float m_amplification;
	bool m_reversed;
	float m_frequency;
	sample_rate_t m_sampleRate;

signals:
	void sampleUpdated();
};

#endif