/*
 * AsyncWorkerPool.h
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

#ifndef LMMS_ASYNC_WORKER_POOL_H
#define LMMS_ASYNC_WORKER_POOL_H

#include <condition_variable>
#include <emmintrin.h>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

namespace lmms {
class AsyncWorkerPool
{
public:
	AsyncWorkerPool(unsigned int numWorkers = std::thread::hardware_concurrency());
	~AsyncWorkerPool();

	//! Enqueue function `fn` with arguments `args` to be ran asynchronously.
	template <typename Fn, typename... Args>
	auto enqueue(Fn&& fn, Args&&... args) -> std::future<std::invoke_result_t<Fn, Args...>>
	{
		using ReturnType = std::invoke_result_t<Fn, Args...>;
		using PackagedTask = std::packaged_task<ReturnType()>;

		auto callable = [fn = std::move(fn), args = std::make_tuple(std::forward<Args>(args)...)] {
			return std::apply(fn, args);
		};

		auto packagedTask = std::make_shared<PackagedTask>(std::move(callable));
		auto task = [packagedTask] { return (*packagedTask)(); };

		{
			const auto lock = std::lock_guard{m_runMutex};
			m_tasks.emplace(std::move(task));
			m_runCond.notify_one();
		}

		return packagedTask->get_future();
	}

private:
	void process();
	std::queue<std::function<void()>> m_tasks;
	std::vector<std::thread> m_workers;
	std::condition_variable m_runCond, m_waitCond;
	std::mutex m_runMutex;
	bool m_done = false;
};
} // namespace lmms

#endif // LMMS_ASYNC_WORKER_POOL_H
