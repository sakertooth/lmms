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

#if defined LMMS_BUILD_LINUX || defined LMMS_BUILD_APPLE || defined LMMS_BUILD_OPENBSD || defined LMMS_BUILD_FREEBSD
#include <fcntl.h>
#include <sys/mman.h>
#endif

namespace lmms {

#if defined LMMS_BUILD_LINUX || defined LMMS_BUILD_APPLE || defined LMMS_BUILD_OPENBSD || defined LMMS_BUILD_FREEBSD
MemoryMappedFile::MemoryMappedFile(const std::filesystem::path& path)
	: m_path(path)
	, m_size(std::filesystem::file_size(path))
{
	const auto fd = open(path.c_str(), O_RDWR);
	if (fd == -1) { throw std::runtime_error{"Failed to open file"}; }

	m_data = static_cast<char*>(mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, m_pos));
	if (m_data == MAP_FAILED) { throw std::runtime_error{"Failed to create memory mapping of file"}; }

	close(fd);
}

MemoryMappedFile::~MemoryMappedFile()
{
	munmap(m_data, m_size);
}
#endif

#if defined LMMS_BUID_WIN32
MemoryMappedFile::MemoryMappedFile(const std::filesystem::path& path)
	: m_path(path)
	, m_size(std::filesystem::file_size(path))
	, m_fileHandle(CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
		  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr))
	, m_mapHandle(CreateFileMapping(m_fileHandle, nullptr, PAGE_EXECUTE_READWRITE, 0, 0, nullptr))
	, m_data(static_cast<char*>(MapViewOfFile(m_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, m_size)))
{
}

MemoryMappedFile::~MemoryMappedFile()
{
	UnmapViewOfFile(m_data);
	CloseHandle(m_mapHandle);
	CloseHandle(m_fileHandle);
}
#endif

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& mmapFile) noexcept
	: m_path(std::move(mmapFile.m_path))
	, m_pos(std::exchange(mmapFile.m_pos, 0))
	, m_size(std::exchange(mmapFile.m_size, 0))
	, m_data(std::exchange(mmapFile.m_data, nullptr))
#ifdef LMMS_BUILD_WIN32
	, m_mapHandle(std::exchange(mmapFile.m_mapHandle, INVALID_HANDLE_VALUE))
	, m_fileHandle(std::exchange(mmapFile.m_fileHandle, INVALID_HANDLE_VALUE))
#endif
{
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& mmapFile) noexcept
{
	m_path = std::move(mmapFile.m_path);
	m_pos = std::exchange(mmapFile.m_pos, 0);
	m_size = std::exchange(mmapFile.m_size, 0);
	m_data = std::exchange(mmapFile.m_data, nullptr);
#ifdef LMMS_BUILD_WIN32
	m_mapHandle = std::exchange(mmapFile.m_mapHandle, INVALID_HANDLE_VALUE);
	m_fileHandle = std::exchange(mmapFile.m_fileHandle, INVALID_HANDLE_VALUE);
#endif
	return *this;
}

auto MemoryMappedFile::read(void* ptr, std::size_t count) -> std::size_t
{
	if (m_data == nullptr || m_pos >= m_size) { return 0; }

	const auto numBytesToRead = std::min(count, m_size - m_pos);
	std::copy_n(m_data + m_pos, numBytesToRead, static_cast<char*>(ptr));

	m_pos += numBytesToRead;
	return numBytesToRead;
}

auto MemoryMappedFile::write(const void* ptr, std::size_t count) -> std::size_t
{
	if (m_data == nullptr || m_pos >= m_size) { return 0; }

	const auto numBytesToWrite = std::min(count, m_size - m_pos);
	std::copy_n(static_cast<const char*>(ptr), numBytesToWrite, m_data + m_pos);

	m_pos += numBytesToWrite;
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
		m_pos = m_size + offset;
		break;
	}

	return m_pos;
}

} // namespace lmms