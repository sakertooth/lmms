/*
 * SampleStream.h
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

#ifndef LMMS_SAMPLE_STREAM_H
#define LMMS_SAMPLE_STREAM_H

#include <filesystem>
#include <sndfile.h>

#include "MemoryMappedFile.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleStream
{
public:
	SampleStream(const std::filesystem::path& audioFilePath)
		: m_memoryMappedFile(audioFilePath)
		, m_sfVirtualIo(SF_VIRTUAL_IO{.get_filelen = &SampleStream::filelen,
			  .seek = &SampleStream::seek,
			  .read = &SampleStream::read,
			  .write = &SampleStream::write,
			  .tell = &SampleStream::tell})
		, m_sndfile(sf_open_virtual(&m_sfVirtualIo, SFM_READ, &m_sfInfo, &m_memoryMappedFile))
	{
	}

	SampleStream(const SampleStream&) = delete;
	SampleStream& operator=(const SampleStream&) = delete;

	SampleStream(SampleStream&& stream) noexcept
		: m_memoryMappedFile(std::move(stream.m_memoryMappedFile))
		, m_sfVirtualIo(std::exchange(stream.m_sfVirtualIo, SF_VIRTUAL_IO{}))
		, m_sfInfo(std::exchange(stream.m_sfInfo, SF_INFO{}))
		, m_sndfile(sf_open_virtual(&m_sfVirtualIo, SFM_READ, &m_sfInfo, &m_memoryMappedFile))
	{
		sf_close(stream.m_sndfile);
		stream.m_sndfile = nullptr;
	}

	SampleStream& operator=(SampleStream&& stream) noexcept
	{
		m_memoryMappedFile = std::move(stream.m_memoryMappedFile);
		m_sfVirtualIo = std::exchange(stream.m_sfVirtualIo, SF_VIRTUAL_IO{});
		m_sfInfo = std::exchange(stream.m_sfInfo, SF_INFO{});
		m_sndfile = sf_open_virtual(&m_sfVirtualIo, SFM_READ, &m_sfInfo, &m_memoryMappedFile);

		sf_close(stream.m_sndfile);
		stream.m_sndfile = nullptr;

		return *this;
	}

	~SampleStream() { sf_close(m_sndfile); }

	auto next(SampleFrame* ptr, std::size_t count) -> size_t { return sf_readf_float(m_sndfile, ptr->data(), count); }
	auto size() const -> size_t { return m_sfInfo.frames; }
	auto sampleRate() const -> int { return m_sfInfo.samplerate; }

private:
	static auto filelen(void* data) -> sf_count_t { return static_cast<MemoryMappedFile*>(data)->size(); }

	static auto seek(sf_count_t offset, int whence, void* data) -> sf_count_t
	{
		return static_cast<MemoryMappedFile*>(data)->seek(offset, whence);
	}

	static auto read(void* ptr, sf_count_t count, void* data) -> sf_count_t
	{
		return static_cast<MemoryMappedFile*>(data)->read(ptr, count);
	}

	static auto write(const void* ptr, sf_count_t count, void* data) -> sf_count_t
	{
		return static_cast<MemoryMappedFile*>(data)->write(ptr, count);
	}

	static auto tell(void* data) -> sf_count_t { return static_cast<MemoryMappedFile*>(data)->tell(); }

	MemoryMappedFile m_memoryMappedFile;
	SF_VIRTUAL_IO m_sfVirtualIo;
	SF_INFO m_sfInfo;
	SNDFILE* m_sndfile;
};

} // namespace lmms

#endif // LMMS_SAMPLE_BUFFER_H
