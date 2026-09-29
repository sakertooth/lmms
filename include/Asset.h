/*
 * Asset.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_ASSET_H
#define LMMS_ASSET_H

#include <QString>
#include <unordered_map>

#include "LmmsPolyfill.h"

namespace lmms {
/**
 * @brief Represents a resource referenced by a project, including its source,
 * source type, and optional metadata.
 *
 * An Asset can reference a resource stored at a filesystem path or
 * embedded as Base64 data. Metadata is stored as arbitrary key-value
 * pairs and may vary between different assets.
 */
class Asset
{
public:
	enum class Type
	{
		Path,  //!< Resource is referenced by a filesystem path
		Base64 //!< Resource is embedded as Base64-encoded data.
	};

	/**
	 * Constructs an asset from a source and source type.
	 *
	 * @param src The source.
	 * @param type The kind of source.
	 */
	Asset(const QString& src, Type type)
		: m_src{src}
		, m_type{type}
	{
	}

	//! @returns The source.
	auto src() const -> const QString& { return m_src; }

	//! @returns The type of source.
	auto type() const -> Type { return m_type; }

	/**
	 * Returns the metadata associated with the specified key.
	 *
	 * If the key does not exist, an entry is created with an empty value.
	 *
	 * @param key The metadata key.
	 * @return The metadata value associated with the key.
	 */
	auto operator[](const QString& key) -> QString& { return m_metadata[key]; }

	//! @returns True if the assets are equal, false otherwise
	auto operator==(const Asset&) const -> bool = default;

private:
	QString m_src;
	Type m_type;
	std::unordered_map<QString, QString, QtHash<QString>> m_metadata;
};
} // namespace lmms

#endif // LMMS_ASSET_H