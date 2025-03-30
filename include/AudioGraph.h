/*
 * AudioGraph.h
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

#ifndef LMMS_AUDIO_GRAPH_H
#define LMMS_AUDIO_GRAPH_H

#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "MixHelpers.h"
#include "SampleFrame.h"
#include "SpscLockfreeQueue.h"

namespace lmms {
class AudioGraph
{
public:
	class Node
	{
	protected:
		class Token
		{
		};

	public:
		enum class State
		{
			Idle,
			Queued,
			Processing,
			Processed,
			Completed
		};

		Node(Token);
		virtual ~Node() noexcept;

		Node(const Node&) = delete;
		Node(Node&&) = delete;
		Node& operator=(const Node&) = delete;
		Node& operator=(Node&&) = delete;

		template <typename T> static std::unique_ptr<T> create()
		{
			auto node = std::make_unique<T>(Token{});
			AudioGraph::inst().add(node);
			return node;
		}

		void send(Node* node) { sendImpl(node, node->m_buffer.data(), m_buffer.data(), m_buffer.size()); }
		void send(SampleFrame* dst, std::size_t size) { sendImpl(nullptr, dst, m_buffer.data(), size); }
		void process() { processImpl(m_buffer.data(), m_buffer.size()); }
		bool finished() { return finishedImpl(); }

	private:
		virtual void sendImpl(Node* node, SampleFrame* dst, const SampleFrame* src, std::size_t size)
		{
			MixHelpers::add(m_buffer.data(), node->m_buffer.data(), m_buffer.size());
		}

		virtual void processImpl(SampleFrame* dst, std::size_t size) = 0;
		virtual bool finishedImpl() { return false; }

		std::atomic<State> m_state = State::Idle;
		std::vector<SampleFrame> m_buffer;
		AudioGraph* m_graph = nullptr;

		friend class AudioGraph;
	};

	AudioGraph();
	AudioGraph(const AudioGraph&) = delete;
	AudioGraph(AudioGraph&&) = delete;
	AudioGraph& operator=(const AudioGraph&) = delete;
	AudioGraph& operator=(AudioGraph&&) = delete;
	~AudioGraph() noexcept;

	void add(Node* node);
	void remove(Node* node);
	void route(Node* from, Node* to);
	void unroute(Node* from, Node* to);
	void reroute(Node* oldFrom, Node* oldTo, Node* newFrom, Node* newTo);
	void process();

	static AudioGraph& inst();

private:
	class Worker
	{
	public:
		Worker(AudioGraph* graph);
		~Worker() noexcept;

		Worker(Worker&& worker) noexcept;
		Worker& operator=(Worker&&) noexcept;

		Worker(const Worker&) = delete;
		Worker& operator=(const Worker&) = delete;

	private:
		void runWorker(AudioGraph* graph);

		static constexpr auto MaxWorkPerWorker = 512;
		SpscLockfreeQueue<Node*> m_queue;
		std::thread m_thread;
		friend class AudioGraph;
	};

	std::vector<Node*> m_nodes;
	std::unordered_map<Node*, std::vector<Node*>> m_dependencies;
	std::unordered_map<Node*, std::vector<Node*>> m_dependents;

	std::vector<Worker> m_workers;
	std::atomic<bool> m_quit = false;
	std::atomic<std::size_t> m_nodesLeftToProcess = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_GRAPH_H