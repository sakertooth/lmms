/*
 * AutomatableModelTest.cpp
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "QTestSuite.h"
#include <QFileDialog>
#include "SampleBufferV2.h"
#include "SampleBufferCache.h"

#include <QString>
#include <iostream>
#include <chrono>

class SampleCacheTest : QTestSuite
{
	Q_OBJECT
private slots:
	void createBufferTest()
	{ 
		auto start = std::chrono::steady_clock::now();
		m_buffer = SampleBufferV2("/home/saker/Desktop/aquatic.wav");
		auto end = std::chrono::steady_clock::now();

		std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms to create buffer.\n";
		std::cout << "sample rate: " << m_buffer.sampleRate() << '\n';
		std::cout << "num of frames: " << m_buffer.data().size() << '\n';
	}

	void cacheTest() 
	{
		SampleBufferCache cache;

		if (m_buffer.hasFilePath()) 
		{
			std::vector<SampleBufferV2> buffers;
			auto buffer = cache.insert(m_buffer.filePath(), new SampleBufferV2(std::move(m_buffer)));
			
			if (buffer)
			{
				std::cout << "A raw pointer to the SampleBuffer was moved into the cache\n";
			}
			else {
				std::cout << "Could not move a SampleBuffer* into the cache\n";
			}
			
		}

		std::cout << "cache size here should be 0: " << cache.size() << '\n';
	}
private:
	SampleBufferV2 m_buffer;
} SampleCacheTest;

#include "SampleCacheTest.moc"
