/*
 * AudioFile.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "AudioFile.h"

#include <utility>

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"

namespace lmms {
AudioFile::AudioFile(const std::filesystem::path& path, Mode mode)
	: m_sndfile(openFileHandle(path, mode, m_info))
{
	if (!m_sndfile) { throw std::runtime_error{"Failed to load audio file: " + std::string{sf_strerror(m_sndfile)}}; }
}

AudioFile::AudioFile(AudioFile&& other) noexcept
	: m_info(std::exchange(other.m_info, SF_INFO{}))
	, m_sndfile(std::exchange(other.m_sndfile, nullptr))
{
}

AudioFile& AudioFile::operator=(AudioFile&& other) noexcept
{
	m_sndfile = std::exchange(other.m_sndfile, nullptr);
	m_info = std::exchange(other.m_info, SF_INFO{});
	return *this;
}

AudioFile::~AudioFile()
{
	sf_close(m_sndfile);
}

std::size_t AudioFile::read(float* dst, std::size_t size)
{
	return sf_read_float(m_sndfile, dst, static_cast<sf_count_t>(size));
}

std::size_t AudioFile::write(const float* src, std::size_t size)
{
	return sf_write_float(m_sndfile, src, static_cast<sf_count_t>(size));
}

std::size_t AudioFile::seek(std::size_t offset, Seek seek)
{
	// TODO C++23: Use std::to_underlying
	return sf_seek(m_sndfile, static_cast<sf_count_t>(offset), static_cast<int>(seek));
}

SNDFILE* AudioFile::openFileHandle(const std::filesystem::path& path, Mode mode, SF_INFO& info)
{
	SNDFILE* handle = nullptr;

	// TODO C++23: Use std::to_underlying
#ifdef LMMS_BUILD_WIN32
	handle = sf_wchar_open(path.c_str(), static_cast<int>(mode), &info);
#else
	handle = sf_open(path.c_str(), static_cast<int>(mode), &info);
#endif
	return handle;
}

AudioFile::BufferedAudioFile AudioFile::load(const std::filesystem::path& path)
{
	if (SF_INFO info; auto handle = openFileHandle(path, Mode::Read, info))
	{
		std::vector<float> samples(info.frames * info.channels);
		sf_read_float(handle, samples.data(), static_cast<sf_count_t>(samples.size()));
		return BufferedAudioFile{.samples = samples,
			.sampleRate = static_cast<unsigned int>(info.samplerate),
			.numChannels = static_cast<unsigned int>(info.channels)};
	}
	else
	{
		int16_t* dataPtr = nullptr;

		auto ds = DrumSynth{};
		const auto engineRate = Engine::audioEngine()->outputSampleRate();

#ifdef LMMS_BUILD_WIN32
		const auto frames = ds.GetDSFileSamples(QString::fromStdWString(path.native()), dataPtr, DEFAULT_CHANNELS, engineRate);
#else
		const auto frames = ds.GetDSFileSamples(QString::fromStdString(path.native()), dataPtr, DEFAULT_CHANNELS, engineRate);
#endif

		const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

		if (frames <= 0 || !data) { throw std::runtime_error{"Could not load DrumSynth file"}; }

		auto result = std::vector<float>(frames);
		src_short_to_float_array(data.get(), result.data(), frames * DEFAULT_CHANNELS);

		return BufferedAudioFile{.samples = std::move(result),
			.sampleRate = static_cast<unsigned int>(engineRate),
			.numChannels = DEFAULT_CHANNELS};
	}
}
}