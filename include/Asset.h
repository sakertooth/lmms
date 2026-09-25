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

#include <filesystem>
#include <variant>

namespace lmms {

struct FileAsset
{
	std::filesystem::path path;
	auto operator==(const FileAsset&) const -> bool = default;
};

struct Base64Asset
{
	std::string data;
	auto operator==(const Base64Asset&) const -> bool = default;
};

struct FileAssetEntry
{
	std::weak_ptr<void> resource;
	std::filesystem::file_time_type lastWriteTime;
};

struct Base64AssetEntry
{
	std::weak_ptr<void> resource;
};

using Asset = std::variant<FileAsset, Base64Asset>;
using AssetEntry = std::variant<FileAssetEntry, Base64AssetEntry>;

} // namespace lmms

template <> struct std::hash<lmms::FileAsset>
{
	auto operator()(const lmms::FileAsset& asset) const -> std::size_t
	{ return std::hash<std::filesystem::path>{}(asset.path); }
};

template <> struct std::hash<lmms::Base64Asset>
{
	auto operator()(const lmms::Base64Asset& asset) const -> std::size_t
	{ return std::hash<std::string>{}(asset.data); }
};

#endif // LMMS_ASSET_H