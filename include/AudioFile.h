/*
 * AudioFile.h
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

#ifndef LMMS_AUDIO_FILE_H
#define LMMS_AUDIO_FILE_H

#include <filesystem>
#include <sndfile.h>
#include <vector>

namespace lmms {
/**
	An abstraction for audio files on disk.
 */
class AudioFile
{
public:
	enum class Mode : int
	{
		Read = SFM_READ,		//! Open the file for reading
		Write = SFM_WRITE,		//! Open the file for writing
		ReadAndWrite = SFM_RDWR //! Open the file for both reading and writing
	};

	enum class Seek : int
	{
		Set = SF_SEEK_SET,		//! Relative to the start of the file
		Current = SF_SEEK_CUR,	//! Relative to the current position of the file
		End = SF_SEEK_END		//! Relative to the end of the file
	};

	struct BufferedAudioFile
	{
		std::vector<float> samples;
		unsigned int sampleRate;
		unsigned int numChannels;
		auto numFrames() const { return samples.size() / numChannels; }
	};

	AudioFile(const AudioFile&) = delete;
	AudioFile& operator=(const AudioFile&) = delete;

	AudioFile(AudioFile&&) noexcept;
	AudioFile& operator=(AudioFile&&) noexcept;

	/**
		Opens an audio file at the specified `path` with a certain `mode`.
		Throws a `std::runtime_error` exception on error.
	*/
	AudioFile(const std::filesystem::path& path, Mode mode);

	/**
		Closes the opened audio file.
	 */
	~AudioFile();

	/**
		Reads `size` samples from the audio file into `dst`.
		Returns the number of samples read.
	 */
	auto read(float* dst, std::size_t size) -> std::size_t;

	/**
		Writes `size` samples from the `src` into the audio file.
		Returns the number of samples written.
	 */
	auto write(const float* src, std::size_t size) -> std::size_t;

	/**
		Move to a new position within the audio file by `offset` frames.
		A frame is a number of samples equal to how many channels the audio
		file has.

		The `seek` option specifies how the offset should be applied (i.e.,
		relative to the start of the file for `Set`, the current position for `Current`,
		or the end of the file for `End`)
	 */
	auto seek(std::size_t offset, Seek seek) -> std::size_t;

	/**
		Return the number of frames within the audio file.
	 */
	auto numFrames() const -> std::size_t { return m_info.frames; }

	/**
		Return the number of channels the audio file has.
	 */
	auto numChannels() const -> std::size_t { return m_info.channels; }

	/**
		Return the sample rate the audio file is meant to be played at.
	 */
	auto sampleRate() const -> std::uint32_t { return m_info.samplerate; }

	/**
		Loads an audio file in its entirety from the specified `path`.
		This can also load DrumSynth files (.ds), which cannot
		be used like a regular audio file.

		Returns the loaded audio file along with its sample rate and the number
		of channels within it.

		Throws a `std::runtime_error` exception on error.
	 */
	static auto load(const std::filesystem::path& path) -> BufferedAudioFile;

private:
	static SNDFILE* openFileHandle(const std::filesystem::path& path, Mode mode, SF_INFO& info);
	SF_INFO m_info = SF_INFO{};
	SNDFILE* m_sndfile = nullptr;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_H
