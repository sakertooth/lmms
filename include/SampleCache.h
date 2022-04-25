/*
 * SampleCache.h - Used to cache sample frames
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

#ifndef SAMPLE_CACHE_H
#define SAMPLE_CACHE_H

#include <map>
#include <memory>
#include <QString>

#include "lmms_basics.h"

 /**
  * @brief
  * SampleCacheEntry encapsulates sample frame data, along with the number of frames and a counter.
  * data is a std::unique_ptr<sampleFrame> because the frames should not be shared among buffers (maybe not?).
  */
struct SampleCacheEntry
{
	std::unique_ptr<sampleFrame> data;
	f_cnt_t numFrames;
	int cacheRefCounter;
};

/**
 * @brief
 * SampleCache caches sampleFrame*'s, which are encapsulated in SampleCacheEntry.
 *
 * When creating a new sample buffer from the QString that is already in the cache,
 * it should copy it from the frames in the entry rather than reload it into memory.
 *
 * The counters are used to track how many sample buffers refer to the entry.
 * When the counter reaches zero, the entry is removed from the cache.
 */
class SampleCache
{
public:
	bool addCacheEntry(const QString &id, sampleFrame *data, f_cnt_t numFrames);
	SampleCacheEntry *getCacheEntry(const QString &id);

	bool contains(const QString &id);

	void incrementCacheRefCounter(const QString &id);
	void decrementCacheRefCounter(const QString &id);

private:
	std::map<QString, SampleCacheEntry> m_cache;
};

#endif