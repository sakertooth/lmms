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

#include <QDateTime>
#include <QFileInfo>
#include <QUuid>
#include <typeindex>

#include "Asset.h"
#include "LmmsPolyfill.h"

namespace lmms {
class AssetManager
{
public:
	auto getAssetId(const Asset& asset) -> QUuid
	{
		for (const auto& [uuid, otherAsset] : m_assets)
		{
			if (asset == otherAsset) { return uuid; }
		}

		const auto uuid = QUuid::createUuid();
		m_assets[uuid] = asset;
		return uuid;
	}

	auto getAsset(const QUuid& assetId) const -> const Asset*
	{
		const auto it = m_assets.find(assetId);
		return it == m_assets.end() ? nullptr : &it->second;
	}

	template <typename T> auto getResource(const Asset& asset) -> std::shared_ptr<const T>
	{
		const auto id = getAssetId(asset);
		return getResource<T>(id, &asset);
	}

	template <typename T>
	auto getResource(const QUuid& assetId, const Asset* assetHint = nullptr) -> std::shared_ptr<const T>
	{
		auto& resourceEntries = m_resources[assetId];
		const auto [it, inserted] = resourceEntries.try_emplace(typeid(T), ResourceEntry{});

		auto& entry = it->second;

		const auto asset = assetHint ? assetHint : getAsset(assetId);
		assert(getAsset(assetId) == assetHint);

		if (!inserted
			&& ((asset->type() == Asset::Type::Path && entry.modificationTime == QFileInfo{asset->src()}.lastModified())
				|| asset->type() == Asset::Type::Base64))
		{
			return std::static_pointer_cast<T>(entry.resource);
		}

		if (asset->type() == Asset::Type::Path)
		{
			const auto resource = T::fromFile(asset->src(), *this);
			entry.resource = resource;
			entry.modificationTime = QFileInfo{asset->src()}.lastModified();
			return resource;
		}
		else if (asset->type() == Asset::Type::Base64)
		{
			const auto resource = T::fromBase64(asset->src(), *this);
			entry.resource = resource;
			return resource;
		}
	}

	template <typename T> auto addResource(const Asset& asset, std::shared_ptr<const T> resource) -> bool
	{
		auto& resourceEntries = m_resources[getAssetId(asset)];
		const auto [it, inserted] = resourceEntries.try_emplace(typeid(T), ResourceEntry{resource});
		return inserted;
	}

private:
	struct ResourceEntry
	{
		std::weak_ptr<const void> resource;
		std::optional<QDateTime> modificationTime;
	};

	std::unordered_map<QUuid, Asset, QtHash<QUuid>> m_assets;
	std::unordered_map<QUuid, std::unordered_map<std::type_index, ResourceEntry>, QtHash<QUuid>> m_resources;
};
} // namespace lmms

#endif // LMMS_ASSET_H