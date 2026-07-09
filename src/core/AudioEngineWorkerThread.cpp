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

AudioEngineWorkerThread::AudioEngineWorkerThread(AudioEngine* audioEngine)
	: QThread(audioEngine)
{
}

AudioEngineWorkerThread::~AudioEngineWorkerThread()
{
}

void AudioEngineWorkerThread::quit()
{
	m_quit.store(true, std::memory_order_relaxed);
}


void AudioEngineWorkerThread::addJob(ThreadableJob *_job)
{
	// TODO: Reimplement using dependency graph
}

void AudioEngineWorkerThread::reset()
{
	// TODO: Reimplement using dependency graph
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

} // namespace lmms
