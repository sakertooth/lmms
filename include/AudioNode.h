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
class AudioNode
{
public:
	static constexpr auto DefaultBufferSize = 256;
	static constexpr auto MaxBufferSize = 4096;

	using Buffer = ArrayVector<sampleFrame, MaxBufferSize>;

	AudioNode() = default;

	AudioNode(const AudioNode&) = delete;
	AudioNode(AudioNode&&) = delete;

	AudioNode& operator=(const AudioNode&) = delete;
	AudioNode& operator=(AudioNode&&) = delete;

	//! Disconnects from all other nodes it has connections to.
	virtual ~AudioNode() noexcept;

	//! Render `size` sample frames and store it in `dest`.
	//! Precondition: `size <= MaxBufferSize == true`.
	virtual void render(sampleFrame* dest, std::size_t size) = 0;

	//! Optionally invokes post processing on `size` sample frames within `src` and mixes the output into `dest`.
	//! `recipient` represents the intended recipient the audio processed is going to.
	//! The default implementation does no post processing and simply mixes `src` into `dest` unchanged.
	//! Precondition: `size <= MaxBufferSize == true`.
	virtual void send(sampleFrame* dest, const sampleFrame* src, std::size_t size, AudioNode& recipient);

	//! Enqueue a topological ordering of calls to `render` for this node and its dependencies into `pool`.
	//! Returns a future that returns the `Buffer` output when this node finishes executing within `pool`.
	//! Precondition: `size <= MaxBufferSize` is `true`.
	auto pull(AsyncWorkerPool& pool, std::size_t size) -> std::future<Buffer>;

	//! Connect the output from this node to the input of `node`.
	void connect(AudioNode* node);

	//! Disconnect the output from this node to the input of `node`.
	void disconnect(AudioNode* node);

private:
	auto process(std::size_t size, std::size_t numDependencies) -> Buffer;
	Buffer m_buffer;
	size_t m_numInputs = 0;
	std::vector<AudioNode*> m_dependencies, m_destinations;
	std::mutex m_mutex;
};
} // namespace lmms

#endif // LMMS_AUDIO_NODE_H
