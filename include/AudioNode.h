/*
 * AudioNode.h
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

#ifndef LMMS_AUDIO_NODE_H
#define LMMS_AUDIO_NODE_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

#include "lmms_basics.h"

namespace lmms {
class AudioNode
{
public:
	struct Buffer
	{
		const sampleFrame* buffer;
		size_t size;
	};

	class Processor
	{
		Processor(unsigned int numWorkers = std::thread::hardware_concurrency());
		~Processor();
		void process(AudioNode& target);

	private:
		void populateQueue(AudioNode& target);
		void run();

		AudioNode* m_target;
		std::list<AudioNode*> m_queue;
		std::vector<std::thread> m_workers;

		std::condition_variable m_runCond;
		std::mutex m_runMutex;

		std::atomic<bool> m_done = false;
		std::atomic<bool> m_targetComplete = false;
	};

	AudioNode(size_t size);
	~AudioNode();

	//! Render audio for an audio period and store results in `dest` of size `size`.
	virtual void render(const sampleFrame* dest, size_t size) = 0;

	//! Mix in `src` of size `size` as input to this node's buffer.
	//! Mixes only up to the size of the node.
	void push(const sampleFrame* src, size_t size);

	//! Connect output from this node to the input of `dest`.
	void connect(AudioNode* dest);

	//! Disconnect output from this node from the input of `dest`. 
	void disconnect(AudioNode* dest);

private:
	auto process() -> Buffer;
	std::vector<sampleFrame> m_buffer;
	std::vector<AudioNode*> m_dependencies;
	std::vector<AudioNode*> m_destinations;
	std::atomic<int> m_numInputs = 0;
	std::atomic<int> m_numDependencies = 0;
	std::mutex m_connectionMutex, m_processMutex;
};
} // namespace lmms

#endif // LMMS_AUDIO_NODE_H
