/*
 * Metronome.cpp
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

#include "Metronome.h"

#include "ArrayVector.h"
#include "MixHelpers.h"
#include "Mixer.h"
#include "Song.h"

namespace lmms {
Metronome::Metronome()
	: m_strongBeat(QString{"misc/metronome02.ogg"})
	, m_weakBeat(QString{"misc/metronome01.ogg"})
{
	connect(Engine::mixer()->mixerChannel(0));
}

void Metronome::render(sampleFrame* dest, std::size_t size)
{
	const auto song = Engine::getSong();
	const auto ticks = song->getPlayPos().getTicks();
	const auto ticksPerBeat = song->getPlayPos().ticksPerBeat(song->getTimeSigModel());

	const auto currentPlayMode = song->playMode();
	const auto metronomeSupported = currentPlayMode == Song::PlayMode::MidiClip
		|| currentPlayMode == Song::PlayMode::Song || currentPlayMode == Song::PlayMode::Pattern;
	if (!metronomeSupported || !active() || song->isExporting()) { return; }

	if (ticks % ticksPerBeat == 0 && m_prevTicks != ticks)
	{
		m_strongBeat.enabled = song->getBeat() == 0;
		m_weakBeat.enabled = song->getBeat() > 0;
		m_strongBeat.state.setFrameIndex(0);
		m_weakBeat.state.setFrameIndex(0);
	}

	if (m_strongBeat.enabled) { m_strongBeat.sample.play(dest, &m_strongBeat.state, size); }
	else if (m_weakBeat.enabled) { m_weakBeat.sample.play(dest, &m_weakBeat.state, size); }
	m_prevTicks = ticks;
}

bool Metronome::active() const
{
	return m_active.load(std::memory_order_relaxed);
}

void Metronome::setActive(bool active)
{
	m_active.store(active, std::memory_order_relaxed);
}

Metronome::Handle::Handle(const QString& audioFile)
	: sample(audioFile)
{
}

} // namespace lmms
