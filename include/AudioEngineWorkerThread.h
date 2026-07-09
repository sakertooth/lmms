/*
 * AudioEngineWorkerThread.h - declaration of class AudioEngineWorkerThread
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

#ifndef LMMS_AUDIO_ENGINE_WORKER_THREAD_H
#define LMMS_AUDIO_ENGINE_WORKER_THREAD_H

#include <QThread>

#include <atomic>

class QWaitCondition;

namespace lmms
{

class AudioEngine;
class ThreadableJob;

class AudioEngineWorkerThread
{
public:
	AudioEngineWorkerThread();
	~AudioEngineWorkerThread();

	AudioEngineWorkerThread(const AudioEngineWorkerThread&) = delete;
	AudioEngineWorkerThread(AudioEngineWorkerThread&&) = delete;
	AudioEngineWorkerThread& operator=(const AudioEngineWorkerThread&) = delete;
	AudioEngineWorkerThread& operator=(AudioEngineWorkerThread&&) = delete;

	//! Adds @a job as a node to the execution graph
	//! @a job may fail to be added under sufficient load
	static void addJob(ThreadableJob* job);

	//! Adds a dependency relationship to the graph such that @a to will only run if @a from is finished
	//! The relationship may fail to be added under sufficient load
	static void addDependency(ThreadableJob* from, ThreadableJob* to);

	//! Clears the graph to have no nodes and edges
	static void reset();

	//! Executes the nodes within the graph, respecting the dependency relationships made between them
	static void execute();

	// a convenient helper function allowing to pass a container with pointers
	// to ThreadableJob objects
	template<typename T>
	static void fillJobQueue(const T & _vec)
	{
		// TODO: Reimplement using dependency graph
	}

private:
	void run();
	std::atomic<bool> m_quit = false;
	std::thread m_thread;
};

} // namespace lmms

#endif // LMMS_AUDIO_ENGINE_WORKER_THREAD_H
