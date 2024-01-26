/*
 * AsyncWorkerPool.cpp
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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

#include "AsyncWorkerPool.h"

namespace lmms {

AsyncWorkerPool::AsyncWorkerPool(unsigned int numWorkers)
{
	for (unsigned int i = 0; i < numWorkers; ++i)
	{
		m_workers.emplace_back([this] { process(); });
	}
}

AsyncWorkerPool::~AsyncWorkerPool()
{
    {
        const auto lock = std::lock_guard{m_runMutex};
        m_done = true;
    }

    m_runCond.notify_all();
	for (auto& worker : m_workers)
	{
		worker.join();
	}
}

void AsyncWorkerPool::process()
{
	while (!m_done)
	{
        auto task = std::function<void()>{};
		{
			auto lock = std::unique_lock{m_runMutex};
			m_runCond.wait(lock, [this] { return !m_tasks.empty() || m_done; });
			if (m_done) { break; }

			task = std::move(m_tasks.front());
			m_tasks.pop();
		}

        task();
        m_waitCond.notify_one();
	}
}

} // namespace lmms
