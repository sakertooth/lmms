/*
 * AssetManager.h
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

#ifndef LMMS_ASSET_MANAGER_H
#define LMMS_ASSET_MANAGER_H

#include <typeindex>
#include <unordered_map>

#include "Asset.h"
#include "LmmsPolyfill.h"

namespace lmms {
class AssetManager
{
public:
	template <typename T> auto get(const Asset& asset) -> std::shared_ptr<const T>
	{
		auto& assetEntryMap = m_entries[asset];

		return std::visit(
			Overloads{
				[&](const FileAsset& fileAsset) {
					const auto [entry, inserted] = assetEntryMap.try_emplace(typeid(T), FileAssetEntry{});
					const auto latestWriteTime = std::filesystem::last_write_time(fileAsset.path);
					auto& fileAssetEntry = std::get<FileAssetEntry>(entry->second);

					if (!inserted && fileAssetEntry.lastWriteTime == latestWriteTime)
					{
						return fileAssetEntry.resource;
					}

					static_assert(std::is_constructible_v<T, FileAsset>);
					const auto resource = std::make_shared<T>(fileAsset);
					fileAssetEntry.resource = resource;
					fileAssetEntry.lastWriteTime = latestWriteTime;
					return resource;
				},
				[&](const Base64Asset& base64Asset) {
					const auto [entry, inserted] = assetEntryMap.try_emplace(typeid(T), Base64AssetEntry{});
					auto& base64AssetEntry = std::get<Base64AssetEntry>(entry->second);
					if (!inserted) { return base64AssetEntry.resource; }

					static_assert(std::is_constructible_v<T, Base64Asset>);
					const auto resource = std::make_shared<T>(base64Asset);
					base64AssetEntry.resource = resource;
					return resource;
				},
			},
			asset);
	}

private:
	using AssetEntryMap = std::unordered_map<std::type_index, AssetEntry>;
	std::unordered_map<Asset, AssetEntryMap> m_entries;
};
} // namespace lmms

#endif // LMMS_ASSET_H