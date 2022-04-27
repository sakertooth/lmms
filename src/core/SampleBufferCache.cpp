/*
 * SampleBufferCache.cpp - Used to cache sample buffers
 *
 * Copyright (C) 2022 JGHFunRun <JGHFunRun@gmail.com>
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include "SampleBufferCache.h"

std::shared_ptr<const SampleBufferV2> SampleBufferCache::insert(const QString& id, SampleBufferV2* buffer) 
{
	if (!contains(id))
	{
		m_cache.emplace(std::make_pair(id, buffer));
		return (*this)[id];
	}
	return nullptr;
}

bool SampleBufferCache::contains(const QString& id) 
{
	return m_cache.find(id) != m_cache.end();
}

std::size_t SampleBufferCache::size() const 
{
	return m_cache.size();
}

std::shared_ptr<const SampleBufferV2> SampleBufferCache::operator[](const QString& id) 
{
	if (!contains(id)) 
	{
		return nullptr;
	}

	auto deleter = [&](const SampleBufferV2* ptr)
	{
		delete ptr;
		m_cache.erase(id);
	};
	return std::shared_ptr<const SampleBufferV2>(m_cache.at(id), deleter);
}