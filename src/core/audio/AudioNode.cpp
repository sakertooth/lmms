/*
 * AudioNode.cpp
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

#include "AudioNode.h"

#include <algorithm>
#include <cassert>
#include <emmintrin.h>
#include <functional>
#include <unordered_map>

#include "MixHelpers.h"

namespace lmms {

AudioNode::AudioNode(size_t size)
	: m_buffer(size)
{
}

AudioNode::~AudioNode()
{
	for (const auto& dest : m_destinations)
	{
		disconnect(dest);
	}

	for (const auto& dependency : m_dependencies)
	{
		dependency->disconnect(this);
	}
}

auto AudioNode::run() -> Buffer
{
	const auto lock = std::lock_guard{m_processMutex};
	render(m_buffer.data(), m_buffer.size());
	return {m_buffer.data(), m_buffer.size()};
}

void AudioNode::mix(const sampleFrame* src, size_t size)
{
	const auto lock = std::lock_guard{m_processMutex};
	MixHelpers::add(m_buffer.data(), src, m_buffer.size());
	m_numInputs.fetch_add(1, std::memory_order_relaxed);
}

void AudioNode::connect(AudioNode* dest)
{
	const auto lock = std::scoped_lock{m_connectionMutex, dest->m_connectionMutex};
	m_destinations.push_back(dest);
	dest->m_dependencies.push_back(this);
	dest->m_numDependencies.fetch_add(1, std::memory_order_relaxed);
}

void AudioNode::disconnect(AudioNode* dest)
{
	const auto lock = std::scoped_lock{m_connectionMutex, dest->m_connectionMutex};
	m_destinations.erase(std::find(m_destinations.begin(), m_destinations.end(), dest));
	dest->m_dependencies.erase(std::find(dest->m_dependencies.begin(), dest->m_dependencies.end(), this));
	dest->m_numDependencies.fetch_sub(1, std::memory_order_relaxed);
}

AudioNode::Processor::Processor(unsigned int numWorkers)
{
	for (unsigned int i = 0; i < numWorkers; ++i)
	{
		m_workers.emplace_back([this] { run(); });
	}
}

AudioNode::Processor::~Processor()
{
	m_done = true;
	for (auto& worker : m_workers)
	{
		worker.join();
	}
}

auto AudioNode::Processor::process(AudioNode& target) -> Buffer
{
	m_target = &target;

	{
		auto lock = std::lock_guard{m_runMutex};
		populateQueue(target);
	}

	m_runCond.notify_all();

	// TODO C++20: Use std::atomic::wait
	while (!m_targetComplete)
	{
		_mm_pause();
	}

	m_target = nullptr;
	m_targetComplete.store(false, std::memory_order_relaxed);
	m_queue.clear();

	return {m_target->m_buffer.data(), m_target->m_buffer.size()};
}

void AudioNode::Processor::populateQueue(AudioNode& target)
{
	auto temporaryMarks = std::unordered_map<AudioNode*, bool>{};
	auto permanentMarks = std::unordered_map<AudioNode*, bool>{};

	std::function<void(AudioNode*)> visit = [&](AudioNode* node) {
		if (permanentMarks.find(node) != permanentMarks.end()) { return; }
		assert(temporaryMarks.find(node) == temporaryMarks.end());

		temporaryMarks.emplace(node, true);
		for (auto& dependency : node->m_dependencies)
		{
			visit(dependency);
		}

		temporaryMarks.erase(node);
		permanentMarks.emplace(node, true);
		m_queue.push_front(node);
	};

	visit(&target);
}

void AudioNode::Processor::run()
{
	while (!m_done)
	{
		auto nodeToProcess = static_cast<AudioNode*>(nullptr);
		{
			auto lock = std::unique_lock{m_runMutex};
			m_runCond.wait(lock, [this] { return !m_queue.empty() || m_done; });
			if (m_done) { break; }

			nodeToProcess = m_queue.front();
			m_queue.pop_front();
		}

		while (nodeToProcess->m_numInputs < nodeToProcess->m_numDependencies)
		{
			_mm_pause();
		}

		const auto output = nodeToProcess->run();
		for (const auto& dest : nodeToProcess->m_destinations)
		{
			dest->mix(output.buffer, output.size);
		}

		nodeToProcess->m_numInputs.store(0, std::memory_order_relaxed);
		if (nodeToProcess == m_target) { m_targetComplete.store(true, std::memory_order_relaxed); }
	}
}

} // namespace lmms
