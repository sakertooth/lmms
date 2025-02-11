/*
 * MemoryMappedFile.h
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

#ifndef LMMS_MEMORY_MAPPED_FILE_H
#define LMMS_MEMORY_MAPPED_FILE_H

#include <QString>
#include <QVector>
#include <filesystem>

#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT MemoryMappedFile
{
public:
	MemoryMappedFile(const std::filesystem::path& path);
	~MemoryMappedFile();

	auto path() const -> const std::filesystem::path& { return m_path; }
	auto size() const -> std::size_t { return std::filesystem::file_size(m_path); }

	auto read(void* ptr, std::size_t count) -> std::size_t;
	auto write(const void* ptr, std::size_t count) -> std::size_t;

	auto seek(int offset, int whence) -> std::size_t;
	auto tell() -> std::size_t { return m_pos; }

private:
	std::filesystem::path m_path;
	std::size_t m_pos = 0;
	int m_fd = 0;
	char* m_data = nullptr;
};
} // namespace lmms

#endif // LMMS_MEMORY_MAPPED_FILE_H
