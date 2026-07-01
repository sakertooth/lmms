/*
 * AudioTrack.cpp
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_TRACK_H
#define LMMS_AUDIO_TRACK_H

#include <iostream>

#include "AudioBuffer.h"
#include "audio/engine/AudioTrackEvents.h"

namespace lmms {
class AudioTrack
{
public:
	enum class Type
	{
		Sample,
		Instrument,
		Bus,
		Master
	};

	AudioTrack()
		: m_id{s_id++}
	{
	}

	AudioTrack(const AudioTrack&) = default;
	AudioTrack(AudioTrack&&) = delete;
	AudioTrack& operator=(const AudioTrack&) = default;
	AudioTrack& operator=(AudioTrack&&) = delete;
	virtual ~AudioTrack() = default;

	void play(const AudioBuffer* inputBus, AudioBuffer* outputBus) { playImpl(inputBus, outputBus); }

	void processEvent(const AudioTrackEvent& event)
	{
		if (!canProcessEventImpl(event))
		{
#ifdef LMMS_DEBUG
			std::cerr << "Track " << m_id << " received an invalid event\n";
#endif
			return;
		}

		processEventImpl(event);
	}

	auto canProcessEvent(const AudioTrackEvent& event) -> bool { return canProcessEventImpl(event); }

	auto type() const -> Type { return typeImpl(); }

	auto id() const -> int { return m_id; }

private:
	virtual void playImpl(const AudioBuffer* inputBus, AudioBuffer* outputBus) = 0;
	virtual void processEventImpl(const AudioTrackEvent& event) = 0;
	virtual auto canProcessEventImpl(const AudioTrackEvent& event) -> bool = 0;
	virtual auto typeImpl() const -> Type = 0;
	int m_id = 0;
	inline static int s_id = 0;
};
} // namespace lmms

#endif