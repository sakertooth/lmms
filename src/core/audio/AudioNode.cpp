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

#include <iostream>
#include <unordered_map>

#include "MixHelpers.h"

namespace lmms {

AudioNode::~AudioNode() noexcept
{
	for (const auto& dependency : m_dependencies)
	{
		dependency->disconnect(this);
	}

	for (const auto& destination : m_destinations)
	{
		disconnect(destination);
	}
}

void AudioNode::send(sampleFrame* dest, const sampleFrame* src, std::size_t size, AudioNode& recipient)
{
	MixHelpers::add(dest, src, size);
}

void AudioNode::connect(AudioNode* node)
{
	const auto lock = std::scoped_lock{m_mutex, node->m_mutex};
	m_destinations.push_back(node);
	node->m_dependencies.push_back(this);
}

void AudioNode::disconnect(AudioNode* node)
{
	const auto lock = std::scoped_lock{m_mutex, node->m_mutex};
	m_destinations.erase(std::find(m_destinations.begin(), m_destinations.end(), node));
	node->m_dependencies.erase(std::find(node->m_dependencies.begin(), node->m_dependencies.end(), this));
}

auto AudioNode::pull(AsyncWorkerPool& pool, std::size_t size) -> std::future<Buffer>
{
	auto temp = std::unordered_map<AudioNode*, bool>{};
	auto permanent = std::unordered_map<AudioNode*, bool>{};
	auto target = std::future<Buffer>{};

	std::function<void(AudioNode*)> visit = [&](AudioNode* node) {
		const auto lock = std::lock_guard{node->m_mutex};

		if (permanent.find(node) != permanent.end()) { return; }
		assert(temp.find(node) == temp.end() && "Cycle detected");

		temp.emplace(node, true);
		for (const auto& dependency : node->m_dependencies)
		{
			visit(dependency);
		}

		auto task = pool.enqueue(&AudioNode::process, node, size, node->m_dependencies.size());
		if (node == this) { target = std::move(task); }

		temp.erase(node);
		permanent.emplace(node, true);
	};

	visit(this);
	return target;
}

auto AudioNode::process(std::size_t size, std::size_t numDependencies) -> Buffer
{
	while (m_numInputs < numDependencies)
	{
		_mm_pause();
	}

	const auto lock = std::lock_guard{m_mutex};
	render(m_buffer.data(), size);

	for (auto& destination : m_destinations)
	{
		const auto lock = std::lock_guard{destination->m_mutex};
		send(destination->m_buffer.data(), m_buffer.data(), size, *destination);
		++destination->m_numInputs;
	}

	auto result = Buffer{size};
	std::copy_n(m_buffer.begin(), size, result.begin());
	std::fill_n(m_buffer.begin(), size, sampleFrame{});
	m_numInputs = 0;

	return result;
}

} // namespace lmms
