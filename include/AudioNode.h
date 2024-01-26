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

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "ArrayVector.h"
#include "AsyncWorkerPool.h"
#include "lmms_basics.h"

namespace lmms {
/*
	Represents a unit of audio processing.

	`AudioNode` encapsulates some kind of audio processing
	determined by subclasses, and must send/receive data
	from other audio processing units.

	`render` dictates what kind of rendering will be done,
	while `send` allows for the communication of results between nodes.
	These functions are used as an interface specifically for subclasses
	to allow unique audio processing functionality.

	The other provided functions are used for external clients (e.g., audio thread, main thread)
	to interact with the node.
*/
class AudioNode
{
public:
	static constexpr auto DefaultBufferSize = 256;

	AudioNode();

	AudioNode(const AudioNode&) = delete;
	AudioNode(AudioNode&&) = delete;

	AudioNode& operator=(const AudioNode&) = delete;
	AudioNode& operator=(AudioNode&&) = delete;

	//! Disconnects from all other nodes it has connections to.
	virtual ~AudioNode() noexcept;

	//! Render `size` sample frames and store it in `dest`.
	virtual void render(sampleFrame* dest, std::size_t size) = 0;

	//! Optionally invokes post processing on `size` sample frames within `src` and mixes the output into `dest`.
	//! `recipient` represents the intended recipient the audio processed is going to.
	//! The default implementation does no post processing and simply mixes `src` into `dest` unchanged.
	virtual void send(sampleFrame* dest, const sampleFrame* src, std::size_t size, AudioNode& recipient);

	//! Enqueue a topological ordering of the processing for this node and its dependencies into `pool`.
	//! Returns a future that waits for the completion of the node's processing.
	auto execute(AsyncWorkerPool& pool) -> std::future<void>;

	//! Copy the output from this node into `dest`.
	//! The size of `dest` should be equal to the number of frames initially set with `setFramesPerPeriod`.
	void pull(sampleFrame* dest);

	//! Connect the output from this node to the input of `node`.
	void connect(AudioNode* node);

	//! Disconnect the output from this node to the input of `node`.
	void disconnect(AudioNode* node);

	//! Sets the number of frames stored within a node's buffer for all `AudioNode`'s.
	//! This function should only be called once. Subsequent calls will be ignored.
	//! This function should be called before any nodes are created, as without that call, all nodes will have a buffer
	//! of size `0`.
	static void setFramesPerPeriod(std::size_t framesPerPeriod);

private:
	void process(std::size_t size, std::size_t numDependencies);
	size_t m_numInputs = 0;
	std::vector<AudioNode*> m_dependencies, m_destinations;
	std::vector<sampleFrame> m_buffer;
	std::mutex m_mutex;
	inline static size_t s_framesPerPeriod = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_NODE_H
