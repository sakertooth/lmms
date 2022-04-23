/*
 * SampleCache.cpp - Used to cache sample frames
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

#include "SampleCache.h"

std::shared_ptr<SampleFrames> SampleCache::addCacheEntry(const QString& id, sampleFrame* data, f_cnt_t numFrames) 
{
	SampleFrames sFrames = {data, numFrames};
	m_cache.insert({id, std::move(sFrames)});
	return getCacheEntry(id);
}

std::shared_ptr<SampleFrames> SampleCache::getCacheEntry(const QString& id) 
{
	return std::shared_ptr<SampleFrames>(&m_cache.at(id), [this, id](SampleFrames *frames) 
	{
		delete frames->m_data;
		m_cache.erase(id);
	});
}

bool SampleCache::contains(const QString& id) const 
{
	return m_cache.find(id) != m_cache.end();
}
