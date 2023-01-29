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

namespace lmms
{
class LMMS_EXPORT SampleBuffer
{
public:
	using value_type = sampleFrame;
	using reference = sampleFrame&;
	using const_iterator = std::vector<sampleFrame>::const_iterator;
	using const_reverse_iterator = std::vector<sampleFrame>::const_reverse_iterator;
	using difference_type = std::vector<sampleFrame>::difference_type;
	using size_type = std::vector<sampleFrame>::size_type;

	SampleBuffer() = default;
	SampleBuffer(const QString& audioFile);
	SampleBuffer(const QByteArray& base64);
	SampleBuffer(const sampleFrame* data, int numFrames, int sampleRate = Engine::audioEngine()->processingSampleRate());

	const sampleFrame& operator[](size_t index) const;

	QString toBase64() const;

	const QString& audioFile() const { return m_audioFile; }
	sample_rate_t sampleRate() const { return m_sampleRate; }

	const_iterator begin() const { return m_data.begin(); }
	const_iterator end() const { return m_data.end(); }
	const_reverse_iterator rbegin() const { return m_data.rbegin(); }
	const_reverse_iterator rend() const { return m_data.rend(); }

	const sampleFrame* data() const { return m_data.data(); }
	size_type size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }

private:
	bool fileExceedsLimits(const QString& audioFile, bool reportToGui = true) const;
	void decodeSampleSF(const QString& fileName);
	void decodeSampleDS(const QString& fileName);
	void resample(sample_rate_t newSampleRate, bool fromOriginal = true);
private:
	std::vector<sampleFrame> m_data;
	std::vector<sampleFrame> m_originalData;
	QString m_audioFile = "";
	sample_rate_t m_sampleRate = Engine::audioEngine()->processingSampleRate();
	sample_rate_t m_originalSampleRate = Engine::audioEngine()->processingSampleRate();
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
