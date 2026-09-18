/*
 * ResourceCache.h
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

#ifndef LMMS_RESOURCE_CACHE_H
#define LMMS_RESOURCE_CACHE_H

#include <cassert>
#include <filesystem>
#include <list>
#include <unordered_map>
#include <utility>

namespace lmms {

template <typename T> class ResourceCache
{
	struct ActiveEntry;
public:
	inline static constexpr auto DefaultInactiveCapacity = 32;

	ResourceCache(std::size_t inactiveCapacity = DefaultInactiveCapacity)
		: m_inactiveResources(inactiveCapacity)
		, m_inactiveEvictionList(inactiveCapacity)
		, m_inactiveCapacity(inactiveCapacity)
	{ assert(inactiveCapacity >= 1 && "inactive capacity must at least be 1"); }

	template <typename... Args>
	auto fetch(const std::filesystem::path& path, Args&&... args) -> std::shared_ptr<const T>
	{
		const auto latestWriteTime = std::filesystem::last_write_time(path);

		if (const auto activeIt = m_activeResources.find(path); activeIt != m_activeResources.end())
		{
			if (activeIt->second.lastWriteTime == latestWriteTime)
			{
				const auto resource = activeIt->second.resource.lock();
				assert(resource);
				return resource;
			}

			const auto resource = createActiveResource(path, new T{std::forward<Args>(args)...}, latestWriteTime);
			activeIt->second.resource = resource;
			activeIt->second.lastWriteTime = latestWriteTime;
			return resource;
		}
		else if (const auto inactiveIt = m_inactiveResources.find(path); inactiveIt != m_inactiveResources.end())
		{
			if (inactiveIt->second.lastWriteTime == latestWriteTime)
			{
				const auto handle = m_inactiveResources.extract(inactiveIt);
				const auto resource = createActiveResource(path, handle.mapped().resource.release(), latestWriteTime);
				const auto [newIt, inserted] = m_activeResources.emplace(path, resource);
				assert(inserted);
				return resource;
			}

			const auto evictIt = std::find(m_inactiveEvictionList.begin(), m_inactiveEvictionList.end(), inactiveIt);
			assert(evictIt != m_inactiveEvictionList.end());

			m_inactiveEvictionList.erase(evictIt);
			m_inactiveResources.erase(inactiveIt);

			const auto resource = createActiveResource(path, new const T{std::forward<Args>(args)...}, latestWriteTime);
			const auto [newIt, inserted] = m_activeResources.emplace(path, resource);
			assert(inserted);
			return resource;
		}

		const auto resource = createActiveResource(path, new const T{std::forward<Args>(args)...}, latestWriteTime);
		const auto [it, inserted] = m_activeResources.emplace(path, resource);
		assert(inserted);
		return resource;
	}

private:
	auto createActiveResource(const std::filesystem::path& path, const T* ptr,
		std::filesystem::file_time_type latestWriteTime) -> std::shared_ptr<const T>
	{
		return std::shared_ptr<const T>(
			ptr, [this, path, latestWriteTime, lifeToken = std::weak_ptr{m_lifeToken}](auto ptr) {
				// Cache was deleted, just delete the resource
				if (!lifeToken.lock())
				{
					delete ptr;
					return;
				}

				auto it = m_activeResources.find(path);
				assert(it != m_activeResources.end());

				// If its old, we can just delete it
				if (ptr != it->second.resource.lock().get())
				{
					delete ptr;
					return;
				}

				// Otherwise make it inactive
				m_activeResources.erase(it);

				if (m_inactiveResources.size() == m_inactiveCapacity)
				{
					m_inactiveResources.erase(*m_inactiveEvictionList.begin());
					m_inactiveEvictionList.pop_front();
				}

				auto [inactiveIt, inserted]
					= m_inactiveResources.emplace(path, InactiveEntry{std::unique_ptr<const T>{ptr}, latestWriteTime});
				m_inactiveEvictionList.emplace_back(inactiveIt);
			});
	}

	struct ActiveEntry
	{
		std::weak_ptr<const T> resource;
		std::filesystem::file_time_type lastWriteTime;
	};

	struct InactiveEntry
	{
		std::unique_ptr<const T> resource;
		std::filesystem::file_time_type lastWriteTime;
	};

	struct LifeToken
	{
	};

	std::shared_ptr<const LifeToken> m_lifeToken = std::make_shared<LifeToken>();

	std::unordered_map<std::filesystem::path, ActiveEntry> m_activeResources;
	std::unordered_map<std::filesystem::path, InactiveEntry> m_inactiveResources;
	std::list<typename decltype(m_inactiveResources)::iterator> m_inactiveEvictionList;
	std::size_t m_inactiveCapacity;
};
} // namespace lmms

#endif // LMMS_RESOURCE_CACHE_H