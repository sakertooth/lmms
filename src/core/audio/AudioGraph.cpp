/*
 * AudioGraph.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "AudioGraph.h"

#include "AudioEngine.h"
#include "Engine.h"

namespace lmms {

AudioGraph::AudioGraph()
{
	for (auto i = std::size_t{0}; i < std::thread::hardware_concurrency() - 1; ++i)
	{
		m_workers.emplace_back(this);
	}
}

AudioGraph::~AudioGraph() noexcept
{
	m_quit = true;
}

void AudioGraph::add(Node* node)
{
	if (node->m_graph == this) { return; }

	const auto guard = Engine::audioEngine()->requestChangesGuard();
	m_nodes.push_back(node);
	node->m_graph = this;
}

void AudioGraph::remove(Node* node)
{
	if (node->m_graph != this) { return; }

	const auto guard = Engine::audioEngine()->requestChangesGuard();
	m_nodes.erase(std::remove(m_nodes.begin(), m_nodes.end(), node));
	m_dependencies.erase(node);
	m_dependents.erase(node);
	node->m_graph = nullptr;
}

void AudioGraph::route(Node* from, Node* to)
{
	if (from->m_graph != this || to->m_graph != this) { return; }

	const auto guard = Engine::audioEngine()->requestChangesGuard();
	m_dependencies[to].emplace_back(from);
	m_dependents[from].emplace_back(to);
}

void AudioGraph::unroute(Node* from, Node* to)
{
	if (from->m_graph != this || to->m_graph != this) { return; }

	const auto guard = Engine::audioEngine()->requestChangesGuard();
	m_dependencies[to].erase(std::remove(m_dependencies[to].begin(), m_dependencies[to].end(), from));
	m_dependents[from].erase(std::remove(m_dependents[from].begin(), m_dependents[from].end(), to));
}

void AudioGraph::reroute(Node* oldFrom, Node* oldTo, Node* newFrom, Node* newTo)
{
	const auto guard = Engine::audioEngine()->requestChangesGuard();
	unroute(oldFrom, oldTo);
	route(newFrom, newTo);
}

AudioGraph::Node::~Node() noexcept
{
	if (m_graph) { m_graph->remove(this); }
}

AudioGraph& AudioGraph::inst()
{
	static auto s_inst = AudioGraph{};
	return s_inst;
}

void AudioGraph::process()
{
	m_nodesLeftToProcess = m_nodes.size();
	auto nextAssignedWorker = std::size_t{0};

	while (m_nodesLeftToProcess > 0)
	{
		for (auto& node : m_nodes)
		{
			const auto isReady = std::all_of(m_dependencies[node].begin(), m_dependencies[node].end(), [](Node* other) {
				return other->m_state == Node::State::Processed;
			}) && node->m_state == Node::State::Idle;

			if (isReady)
			{
				node->m_state = Node::State::Queued;
				m_workers[nextAssignedWorker].m_queue.push(node);
				nextAssignedWorker = (nextAssignedWorker + 1) % m_workers.size();
			}
		}

		m_nodesLeftToProcess.wait(m_nodesLeftToProcess);
	}

	for (auto& node : m_nodes)
	{
		if (node->finished()) { remove(node); }
		node->m_state = Node::State::Idle;
	}
}

AudioGraph::Worker::Worker(AudioGraph* graph)
	: m_queue(MaxWorkPerWorker)
	, m_thread(&Worker::runWorker, this, graph)
{
}

AudioGraph::Worker::~Worker() noexcept
{
	if (m_thread.joinable()) { m_thread.join(); }
}

AudioGraph::Worker::Worker(Worker&& worker) noexcept
	: m_queue(MaxWorkPerWorker)
	, m_thread(std::move(worker.m_thread))
{
}

AudioGraph::Worker& AudioGraph::Worker::operator=(Worker&& other) noexcept
{
	m_thread = std::move(other.m_thread);
	return *this;
}

void AudioGraph::Worker::runWorker(AudioGraph* graph)
{
	while (!graph->m_quit)
	{
		const auto node = m_queue.pop();

		for (auto& dependency : graph->m_dependencies[node])
		{
			dependency->send(node);
		}

		node->m_state = Node::State::Processing;
		node->process();

		for (auto& dependent : graph->m_dependents[node])
		{
			node->send(dependent);
		}

		node->m_state = Node::State::Processed;
		graph->m_nodesLeftToProcess.fetch_sub(1, std::memory_order_relaxed);
		graph->m_nodesLeftToProcess.notify_one();
	}
}

} // namespace lmms