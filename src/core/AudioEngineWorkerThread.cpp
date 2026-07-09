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
	m_quit.store(true, std::memory_order_relaxed);
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

	fromIt->second.dependents.emplace_back(to);
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
	// TODO: Reimplement using dependency graph
}

void AudioEngineWorkerThread::run()
{
	disableDenormals();

	while (!m_quit.load(std::memory_order_relaxed))
	{
		// TODO: Reimplement using dependency graph
	}
}

auto AudioEngineWorkerThread::WorkQueue::enqueue(WorkNode* node) -> bool
{
	// TODO: Reimplement using dependency graph
}

auto AudioEngineWorkerThread::WorkQueue::dequeue() -> WorkNode*
{
	// TODO: Reimplement using dependency graph
}

auto AudioEngineWorkerThread::WorkQueue::steal() -> WorkNode*
{
	// TODO: Reimplement using dependency graph
}

void AudioEngineWorkerThread::WorkQueue::reset()
{
	m_topIndex.store(0, std::memory_order_relaxed);
	m_bottomIndex.store(0, std::memory_order_relaxed);
}

} // namespace lmms
