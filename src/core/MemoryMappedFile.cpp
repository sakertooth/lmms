/*
 * MemoryMappedFile.cpp
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

#include "MemoryMappedFile.h"

#include <lmmsconfig.h>

#if defined LMMS_BUILD_LINUX || defined LMMS_BUILD_APPLE || defined LMMS_BUILD_OPENBSD || defined LMMS_BUILD_FREEBSD
#include <fcntl.h>
#include <sys/mman.h>
#endif

namespace lmms {

#if defined LMMS_BUILD_LINUX || defined LMMS_BUILD_APPLE || defined LMMS_BUILD_OPENBSD || defined LMMS_BUILD_FREEBSD
MemoryMappedFile::MemoryMappedFile(const std::filesystem::path& path)
	: m_path(path)
	, m_fd(open(path.c_str(), O_RDWR))
	, m_data(static_cast<char*>(mmap(nullptr, size(), PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, m_pos)))
{
}

MemoryMappedFile::~MemoryMappedFile()
{
    munmap(m_data, size());
}
#endif

auto MemoryMappedFile::read(void* ptr, std::size_t count) -> std::size_t
{
	const auto numBytesToRead = std::min(count, size() - m_pos);
	std::copy_n(m_data, numBytesToRead, static_cast<char*>(ptr));
	m_pos += numBytesToRead;
	return numBytesToRead;
}

auto MemoryMappedFile::write(const void* ptr, std::size_t count) -> std::size_t
{
    const auto numBytesToWrite = std::min(count, size() - m_pos);
    std::copy_n(static_cast<const char*>(ptr), numBytesToWrite, m_data + m_pos);
    return numBytesToWrite;
}

auto MemoryMappedFile::seek(int offset, int whence) -> std::size_t
{
    switch (whence)
    {
        case SEEK_SET:
            m_pos = offset;
            break;
        case SEEK_CUR:
            m_pos += offset;
            break;
        case SEEK_END:
            m_pos = size() + offset;
            break;
    }

    return m_pos;
}

} // namespace lmms