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

#include "AudioEngine.h"
#include "Engine.h"

#include <samplerate.h>

#include "lmms_export.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "shared_object.h"
#include "OscillatorConstants.h"
#include "MemoryManager.h"
#include "Note.h"

class QPainter;
class QRect;

namespace lmms
{
class LMMS_EXPORT SampleBuffer
{
public:
	SampleBuffer();
	SampleBuffer(const sampleFrame* data, f_cnt_t frames);
	explicit SampleBuffer(f_cnt_t frames);
	SampleBuffer(const SampleBuffer& other) = delete;
	SampleBuffer(SampleBuffer&& other);
	~SampleBuffer() noexcept;

	SampleBuffer& operator=(const SampleBuffer& other) = delete;
	SampleBuffer& operator=(SampleBuffer&& other);

	static std::shared_ptr<SampleBuffer> createFromAudioFile(const QString& audioFile);
	static std::shared_ptr<SampleBuffer> createFromBase64(const QString& base64);

	void resample(sample_rate_t newSampleRate, bool fromOriginal = true);
	int sampleLength() const;
	QString toBase64() const;

	const QString& audioFile() const { return m_audioFile; }
	const std::vector<sampleFrame>& data() const { return m_data; }
	std::shared_mutex& mutex() const { return m_mutex; }
	sample_rate_t sampleRate() const { return m_sampleRate; }
	const std::unique_ptr<OscillatorConstants::waveform_t>& userAntiAliasWaveTable() const { return m_userAntiAliasWaveTable; }

	f_cnt_t frames() const { return static_cast<f_cnt_t>(m_data.size()); }
public slots:
	void setSampleRate(sample_rate_t sampleRate) { m_sampleRate = sampleRate; }
private:
	void update();
	bool fileExceedsLimits(const QString& audioFile, bool reportToGui = true) const;
	static sample_rate_t audioEngineSampleRate();
	
	void loadFromAudioFile(const QString& audioFile);
	void loadFromBase64(const QString& data);

	void decodeSampleSF(const QString& fileName);
	void decodeSampleDS(const QString& fileName);
	
	void sampleRateChanged();
private:
	QString m_audioFile = "";
	std::vector<sampleFrame> m_data;
	mutable std::shared_mutex m_mutex;
	sample_rate_t m_sampleRate = audioEngineSampleRate();
	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable = std::make_unique<OscillatorConstants::waveform_t>();
	QMetaObject::Connection m_sampleRateChangeConnection;

	// TODO: Move out of SampleBuffer class when sample caching is added
	std::vector<sampleFrame> m_originalData;
	sample_rate_t m_originalSampleRate = audioEngineSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
