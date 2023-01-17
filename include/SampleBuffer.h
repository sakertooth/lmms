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
class LMMS_EXPORT SampleBuffer : public QObject
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

	template<typename... Args>
	static std::shared_ptr<SampleBuffer> create(Args&&... args)
	{
		auto deleter = [](SampleBuffer* obj) { obj->deleteLater(); };
		return std::shared_ptr<SampleBuffer>(new SampleBuffer(std::forward<Args>(args)...), deleter);
	}

	friend void swap(SampleBuffer& first, SampleBuffer& second) noexcept;
	SampleBuffer& operator=(const SampleBuffer that);

	bool play(sampleFrame* dst, Sample::PlaybackState* state, fpp_t frames, float freq, LoopMode loopMode = LoopMode::LoopOff);
	void visualize(QPainter& p, const QRect& dr, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0);

	std::vector<sampleFrame> resample(sample_rate_t srcSR, sample_rate_t dstSR);
	void normalizeSampleRate(sample_rate_t srcSR, bool keepSettings = false);
	sample_t userWaveSample(float sample) const;
	int sampleLength() const;
	QString toBase64() const;

	static const std::array<f_cnt_t, 5>& interpolationMargins();

	const QString& audioFile() const { return m_audioFile; }
	const std::vector<sampleFrame>& data() const { return m_data; }
	std::shared_mutex& mutex() const { return m_mutex; }
	float amplification() const { return m_sample.m_amplification; }
	bool reversed() const { return m_reversed; }
	float frequency() const { return m_sample.m_frequency; }
	sample_rate_t sampleRate() const { return m_sampleRate; }
	const std::unique_ptr<OscillatorConstants::waveform_t>& userAntiAliasWaveTable() const { return m_userAntiAliasWaveTable; }

	f_cnt_t frames() const { return static_cast<f_cnt_t>(m_data.size()); }
	f_cnt_t startFrame() const { return m_sample.m_playMarkers.startFrame; }
	f_cnt_t endFrame() const { return m_sample.m_playMarkers.endFrame; }
	f_cnt_t loopStartFrame() const { return m_sample.m_playMarkers.loopStartFrame; }
	f_cnt_t loopEndFrame() const { return m_sample.m_playMarkers.loopEndFrame; }

signals:
	void sampleUpdated();

public slots:
	void loadFromAudioFile(const QString& audioFile, bool keepSettings = false);
	void loadFromBase64(const QString& data, bool keepSettings = false);

	void setStartFrame(f_cnt_t startFrame) { m_sample.m_playMarkers.startFrame = startFrame; }
	void setEndFrame(f_cnt_t endFrame) { m_sample.m_playMarkers.endFrame = endFrame; }
	void setLoopStartFrame(f_cnt_t loopStart) { m_sample.m_playMarkers.loopStartFrame = loopStart; }
	void setLoopEndFrame(f_cnt_t loopEnd) { m_sample.m_playMarkers.loopEndFrame = loopEnd; }
	void setAllPointFrames(f_cnt_t start, f_cnt_t end, f_cnt_t loopStart, f_cnt_t loopEnd) 
		{ m_sample.m_playMarkers = {start, end, loopStart, loopEnd}; }

	void setAmplification(float amplification);
	void setReversed(bool on);
	void setFrequency(float frequency) { m_sample.m_frequency = frequency; }
	void setSampleRate(sample_rate_t sampleRate) { m_sampleRate = sampleRate; }

	void sampleRateChanged();
private:
	SampleBuffer();
	SampleBuffer(const QString& audioFile, bool isBase64Data = false);
	SampleBuffer(const sampleFrame* data, f_cnt_t frames);
	explicit SampleBuffer(f_cnt_t frames);
	SampleBuffer(const SampleBuffer& orig);

private:
	void update();
	bool fileExceedsLimits(const QString& audioFile, bool reportToGui = true) const;
	sample_rate_t audioEngineSampleRate() const;

	std::pair<std::vector<sampleFrame>, sample_rate_t> decodeSampleSF(const QString& fileName) const;
	std::vector<sampleFrame> decodeSampleDS(const QString& fileName) const;
private:
	QString m_audioFile = "";
	std::vector<sampleFrame> m_data;
	mutable std::shared_mutex m_mutex;
	bool m_reversed = false;
	sample_rate_t m_sampleRate = audioEngineSampleRate();
	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable = std::make_unique<OscillatorConstants::waveform_t>();
	
	// TODO: Slowly moving play information into the Sample class
	// WIll remove once the move is complete.
	Sample m_sample;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
