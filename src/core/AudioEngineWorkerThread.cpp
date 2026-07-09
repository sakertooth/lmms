/*
 * AudioEngineWorkerThread.cpp - implementation of AudioEngineWorkerThread
 *
 * Copyright (c) 2009-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioEngineWorkerThread.h"

#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include "AudioEngine.h"
#include "Hardware.h"
#include "ThreadableJob.h"


namespace lmms
{

AudioEngineWorkerThread::WorkQueue AudioEngineWorkerThread::s_executorWorkQueue;
std::vector<AudioEngineWorkerThread::WorkQueue*> AudioEngineWorkerThread::s_workQueues = {&s_executorWorkQueue};

AudioEngineWorkerThread::AudioEngineWorkerThread()
	: m_thread{[this] { run(); }}
{
	s_workQueues.emplace_back(&m_workQueue);
}

AudioEngineWorkerThread::~AudioEngineWorkerThread()
{
	m_quit.store(true, std::memory_order_release);
	m_thread.join();
}

void AudioEngineWorkerThread::addJob(ThreadableJob *_job)
{
	const auto it = s_workNodes.find(_job);
	if (it == s_workNodes.end())
	{
		s_workNodes.try_emplace(_job, _job);
	}
}

void AudioEngineWorkerThread::addConnection(ThreadableJob* from, ThreadableJob* to)
{
	const auto fromIt = s_workNodes.find(from);
	if (fromIt == s_workNodes.end()) { return; }

	const auto toIt = s_workNodes.find(to);
	if (toIt == s_workNodes.end()) { return; }

	fromIt->second.dependents.emplace_back(&toIt->second);
	++toIt->second.totalDependencies;
}

void AudioEngineWorkerThread::reset()
{
	s_workNodes.clear();
	s_executorWorkQueue.reset();

	for (auto& workQueue : s_workQueues)
	{
		workQueue->reset();
	}
}

void AudioEngineWorkerThread::execute()
{
	auto nextWorkQueue = 0;
	for (auto& [job, workNode] : s_workNodes)
	{
		auto& workQueue = s_workQueues[nextWorkQueue];
		if (workNode.totalDependencies == 0) { workQueue->push(&workNode); }
		nextWorkQueue = (nextWorkQueue + 1) % s_workQueues.size();
		workNode.remainingDependencies.store(workNode.totalDependencies, std::memory_order_relaxed);
	}

	s_jobsCompleted.store(0, std::memory_order_release);
	s_executionFlag.test_and_set(std::memory_order_release);
	s_executionFlag.notify_all();

	while (s_jobsCompleted.load(std::memory_order_relaxed) < s_workNodes.size())
	{
		processQueue(&s_executorWorkQueue);
	}

	s_executionFlag.clear(std::memory_order_release);
	s_executionFlag.notify_all();
}

void AudioEngineWorkerThread::run()
{
	disableDenormals();

	while (!m_quit.load(std::memory_order_relaxed))
	{
		s_executionFlag.wait(false, std::memory_order_acquire);
		processQueue(&m_workQueue);
	}
}

void AudioEngineWorkerThread::processQueue(WorkQueue* workQueue)
{
	auto node = workQueue->pop();

	if (!node)
	{
		for (auto& queue : s_workQueues)
		{
			if (workQueue == queue) { continue; }
			if ((node = queue->steal())) { break; }
		}
	}

	if (!node) { return; }

	node->job->queue();
	node->job->process();
	s_jobsCompleted.fetch_add(1, std::memory_order_relaxed);

	for (auto& dependent : node->dependents)
	{
		if (dependent->remainingDependencies.fetch_sub(1, std::memory_order_relaxed) == 1)
		{
			workQueue->push(dependent);
		}
	}
}

auto AudioEngineWorkerThread::WorkQueue::push(WorkNode* node) -> bool
{
	const auto topIndex = m_topIndex.load(std::memory_order_acquire);
	const auto bottomIndex = m_bottomIndex.load(std::memory_order_acquire);
	if (m_queue.size() <= bottomIndex - topIndex + 1) { return false; }

	m_queue[bottomIndex % m_queue.size()] = node;
	m_bottomIndex.store(bottomIndex + 1, std::memory_order_release);
	return true;
}

auto AudioEngineWorkerThread::WorkQueue::pop() -> WorkNode*
{
	const auto bottomIndex = m_bottomIndex.fetch_sub(1, std::memory_order_acquire) - 1;
	auto topIndex = m_topIndex.load(std::memory_order_acquire);

	if (bottomIndex < topIndex)
	{
		// The queue was empty, so we did not pop anything
		// Correct the bottom index and return nullptr
		m_bottomIndex.store(bottomIndex + 1, std::memory_order_release);
		return nullptr;
	}

	if (bottomIndex > topIndex)
	{
		// Normal case, we have more than one element in the queue
		return m_queue[bottomIndex % m_queue.size()];
	}

	const auto node = m_queue[topIndex % m_queue.size()];
	if (m_topIndex.compare_exchange_strong(topIndex, topIndex + 1))
	{
		// If the owner wins the CAS for the last node, simply return it
		return node;
	}

	// If the owner fails the CAS, one of the stealers got to it first,
	// so increment bottom index to correct it and return nullptr
	m_bottomIndex.store(bottomIndex + 1, std::memory_order_release);
	return nullptr;
}

auto AudioEngineWorkerThread::WorkQueue::steal() -> WorkNode*
{
	auto topIndex = m_topIndex.load(std::memory_order_acquire);
	const auto bottomIndex = m_bottomIndex.load(std::memory_order_acquire);
	if (bottomIndex <= topIndex) { return nullptr; }

	const auto node = m_queue[topIndex % m_queue.size()];
	return m_topIndex.compare_exchange_strong(topIndex, topIndex + 1) ? node : nullptr;
}

void AudioEngineWorkerThread::WorkQueue::reset()
{
	m_topIndex.store(1, std::memory_order_relaxed);
	m_bottomIndex.store(1, std::memory_order_relaxed);
}

} // namespace lmms
