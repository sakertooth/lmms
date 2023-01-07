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
class LMMS_EXPORT SampleBuffer : public QObject, public sharedObject
{
	Q_OBJECT
public:
	enum class LoopMode
	{
		LoopOff, LoopOn, LoopPingPong
	};

	struct PlayMarkers
	{
		f_cnt_t startFrame;
		f_cnt_t endFrame;
		f_cnt_t loopStartFrame;
		f_cnt_t loopEndFrame;
	};

	SampleBuffer();
	SampleBuffer(const QString& audioFile, bool isBase64Data = false);
	SampleBuffer(const sampleFrame* data, f_cnt_t frames);
	explicit SampleBuffer(f_cnt_t frames);
	SampleBuffer(const SampleBuffer& orig);

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;
	SampleBuffer& operator=(const SampleBuffer that);

	bool play(sampleFrame* dst, Sample* state, fpp_t frames, float freq, LoopMode loopMode = LoopMode::LoopOff);
	void visualize(QPainter& p, const QRect& dr, const QRect& clip, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);
	void visualize(QPainter& p, const QRect& dr, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);

	std::vector<sampleFrame> resample(sample_rate_t srcSR, sample_rate_t dstSR);
	void normalizeSampleRate(sample_rate_t srcSR, bool keepSettings = false);
	sample_t userWaveSample(float sample) const;
	int sampleLength() const;
	QString toBase64() const;

	static const std::array<f_cnt_t, 5>& interpolationMargins();

	const QString& audioFile() const;
	const sampleFrame* data() const;
	std::shared_mutex& mutex() const;
	float amplification() const;
	bool reversed() const;
	float frequency() const;
	sample_rate_t sampleRate() const;
	const std::unique_ptr<OscillatorConstants::waveform_t>& userAntiAliasWaveTable() const;

	f_cnt_t frames() const;
	f_cnt_t startFrame() const;
	f_cnt_t endFrame() const;
	f_cnt_t loopStartFrame() const;
	f_cnt_t loopEndFrame() const;

signals:
	void sampleUpdated();

public slots:
	void loadFromAudioFile(const QString& audioFile, bool keepSettings = false);
	void loadFromBase64(const QString& data, bool keepSettings = false);

	void setStartFrame(f_cnt_t startFrame);
	void setEndFrame(f_cnt_t endFrame);
	void setLoopStartFrame(f_cnt_t loopStart);
	void setLoopEndFrame(f_cnt_t loopEnd);
	void setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd);

	void setAmplification(float amplification);
	void setReversed(bool on);
	void setFrequency(float frequency);
	void setSampleRate(sample_rate_t sampleRate);
	void sampleRateChanged();

private:
	void update();
	bool fileExceedsLimits(const QString& audioFile, bool reportToGui = true) const;
	sample_rate_t audioEngineSampleRate() const;

	std::pair<std::vector<sampleFrame>, sample_rate_t> decodeSampleSF(const QString& fileName) const;
	std::vector<sampleFrame> decodeSampleDS(const QString& fileName) const;

	std::vector<sampleFrame> getSampleFragment(
		f_cnt_t index,
		f_cnt_t frames,
		LoopMode loopMode,
		bool* backwards,
		f_cnt_t loopStart,
		f_cnt_t loopEnd,
		f_cnt_t end) const;

	f_cnt_t advance(f_cnt_t playFrame, f_cnt_t frames, LoopMode loopMode, Sample* state);
	f_cnt_t getLoopedIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const;
	f_cnt_t getPingPongIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const;
private:
	QString m_audioFile = "";
	std::vector<sampleFrame> m_data;
	mutable std::shared_mutex m_mutex;
	PlayMarkers m_playMarkers;
	float m_amplification = 1.0f;
	bool m_reversed = false;
	float m_frequency = DefaultBaseFreq;
	sample_rate_t m_sampleRate = audioEngineSampleRate();
	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable = std::make_unique<OscillatorConstants::waveform_t>();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
