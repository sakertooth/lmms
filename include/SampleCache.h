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

#include "SampleFrames.h"
#include "lmms_basics.h"

class SampleCache
{
public:
	std::shared_ptr<SampleFrames> addCacheEntry(const QString& id, sampleFrame* data, f_cnt_t numFrames);
	std::shared_ptr<SampleFrames> getCacheEntry(const QString& id);
	bool contains(const QString& id) const;
private:
	std::map<QString, SampleFrames> m_cache;
};


#endif