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
	public:
		//! Create a processor containing `numWorkers` worker threads.
		Processor(unsigned int numWorkers = std::thread::hardware_concurrency());

		//! Signals to the workers that any processing must cease, and joins all the workers threads.
		~Processor();

		//! Run the processing for `target` and return its output.
		auto process(AudioNode& target) -> Buffer;

	private:
		//! Topologically sort the audio graph, starting from `target`,
		//! and use that as the work queue.
		void populateQueue(AudioNode& target);

		//! Retrieve a node from the work queue, blocking if necessary.
		//! Returns `nullptr` if a node could not be retrieved.
		//! The worker stops if no node could be retrieved.
		auto retrieveNode() -> AudioNode*;

		//! For each destination of the given node, render output for each.
		//! Returns `true` if the target assigned to the processor was the node that was processed.
		//! Returns `false` otherwise.
		void processNode(AudioNode& node);

		//! Run loop for worker thread(s).
		void runWorker();

		AudioNode* m_target = nullptr;
		std::list<AudioNode*> m_queue;
		std::vector<std::thread> m_workers;

		std::condition_variable m_runCond;
		std::mutex m_runMutex;

		std::atomic<bool> m_done = false;
		std::atomic<bool> m_complete = false;
	};

	AudioNode(size_t size);

	AudioNode(const AudioNode&) = delete;
	AudioNode(AudioNode&&) = delete;

	AudioNode& operator=(const AudioNode&) = delete;
	AudioNode& operator=(AudioNode&&) = delete;

	virtual ~AudioNode();

	//! Render and send audio intended for the destination node `dest`.
	//! `input` is the input that was sent to this node by other nodes.
	//! `output` is the place where `output` should be sent to.
	//! Audio sent should be mixed into output, *not* overwritten.
	virtual void render(Buffer input, Buffer output, const AudioNode& dest) = 0;

	//! Connect output from this node to the input of `dest`.
	void connect(AudioNode& dest);

	//! Disconnect output from this node to the input of `dest`.
	void disconnect(AudioNode& dest);

private:
	std::vector<sampleFrame> m_buffer;
	std::vector<AudioNode*> m_dependencies;
	std::vector<AudioNode*> m_destinations;
	std::atomic<int> m_numInputs = 0;
	std::mutex m_renderingMutex, m_connectionMutex;
};
} // namespace lmms

#endif // LMMS_AUDIO_NODE_H