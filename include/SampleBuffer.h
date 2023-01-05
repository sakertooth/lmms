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

#include <memory>
#include <shared_mutex>

#include <QObject>

#include <samplerate.h>

#include "lmms_export.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "shared_object.h"
#include "OscillatorConstants.h"
#include "MemoryManager.h"
#include "Note.h"
#include "Sample.h"

class QPainter;
class QRect;

namespace lmms
{

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
const auto MARGIN = std::array<f_cnt_t, 5>{ 64, 64, 64, 4, 4 };

class LMMS_EXPORT SampleBuffer : public QObject, public sharedObject
{
	Q_OBJECT
	MM_OPERATORS
public:
	enum class LoopMode
	{
		LoopOff,
		LoopOn,
		LoopPingPong
	};
	
	SampleBuffer();
	// constructor which either loads sample _audio_file or decodes
	// base64-data out of string
	SampleBuffer(const QString & audioFile, bool isBase64Data = false);
	SampleBuffer(const sampleFrame * data, const f_cnt_t frames);
	explicit SampleBuffer(const f_cnt_t frames);
	SampleBuffer(const SampleBuffer & orig);

	friend void swap(SampleBuffer & first, SampleBuffer & second) noexcept;
	SampleBuffer& operator=(const SampleBuffer that);

	~SampleBuffer() override;

	bool play(
		sampleFrame * ab,
		Sample * state,
		const fpp_t frames,
		const float freq,
		const LoopMode loopMode = LoopMode::LoopOff
	);

	void visualize(
		QPainter & p,
		const QRect & dr,
		const QRect & clip,
		f_cnt_t fromFrame = 0,
		f_cnt_t toFrame = 0
	);

	void visualize(
		QPainter & p,
		const QRect & dr,
		f_cnt_t fromFrame = 0,
		f_cnt_t toFrame = 0
	);

	const QString & audioFile() const;

	f_cnt_t startFrame() const;
	f_cnt_t endFrame() const;
	f_cnt_t loopStartFrame() const;

	f_cnt_t loopEndFrame() const;
	void setLoopStartFrame(f_cnt_t start);
	void setLoopEndFrame(f_cnt_t end);
	void setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd);

	std::shared_mutex& mutex();

	f_cnt_t frames() const;
	float amplification() const;
	bool reversed() const;
	float frequency() const;
	sample_rate_t sampleRate() const;
	const std::unique_ptr<OscillatorConstants::waveform_t>& userAntiAliasWaveTable() const;

	int sampleLength() const;

	void setFrequency(float freq);
	void setSampleRate(sample_rate_t rate);
	const sampleFrame * data() const;

	QString toBase64() const;

	// protect calls from the GUI to this function with dataReadLock() and
	// dataUnlock()
	SampleBuffer * resample(const sample_rate_t srcSR, const sample_rate_t dstSR);

	void normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings = false);

	// protect calls from the GUI to this function with dataReadLock() and
	// dataUnlock(), out of loops for efficiency
	sample_t userWaveSample(const float sample) const;

public slots:
	void setAudioFile(const QString & audioFile);
	void loadFromBase64(const QString & data);
	void setStartFrame(const lmms::f_cnt_t s);
	void setEndFrame(const lmms::f_cnt_t e);
	void setAmplification(float a);
	void setReversed(bool on);
	void sampleRateChanged();

signals:
	void sampleUpdated();

private:
	sample_rate_t audioEngineSampleRate();

	void update(bool keepSettings = false);

	void convertIntToFloat(int_sample_t * & ibuf, f_cnt_t frames, int channels);
	void directFloatWrite(sample_t * & fbuf, f_cnt_t frames, int channels);

	bool fileExceedsLimits(const QString& audioFile, bool reportToGui = true);

	f_cnt_t decodeSampleSF(
		QString fileName,
		sample_t * & buf,
		ch_cnt_t & channels,
		sample_rate_t & samplerate
	);

	f_cnt_t decodeSampleDS(
		QString fileName,
		int_sample_t * & buf,
		ch_cnt_t & channels,
		sample_rate_t & samplerate
	);

	sampleFrame * getSampleFragment(
		f_cnt_t index,
		f_cnt_t frames,
		LoopMode loopMode,
		sampleFrame * * tmp,
		bool * backwards,
		f_cnt_t loopStart,
		f_cnt_t loopEnd,
		f_cnt_t end
	) const;

	f_cnt_t getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;
	f_cnt_t getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;

private:
	QString m_audioFile = "";
	sampleFrame* m_origData = nullptr;
	f_cnt_t m_origFrames = 0;
	sampleFrame* m_data = nullptr;
	mutable std::shared_mutex m_mutex;
	f_cnt_t m_frames = 0;
	f_cnt_t m_startFrame = 0;
	f_cnt_t m_endFrame = 0;
	f_cnt_t m_loopStartFrame = 0;
	f_cnt_t m_loopEndFrame = 0;
	float m_amplification = 1.0f;
	bool m_reversed = false;
	float m_frequency = DefaultBaseFreq;
	sample_rate_t m_sampleRate = audioEngineSampleRate();
	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
