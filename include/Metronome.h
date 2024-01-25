/*
 * Metronome.h
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

#ifndef LMMS_METRONOME_H
#define LMMS_METRONOME_H

#include "Sample.h"

namespace lmms {
class Metronome : public AudioNode
{
public:
	Metronome(Song& song);

	bool active() const;
	void setActive(bool active);

	void render(sampleFrame* dest, std::size_t size) override;

private:
	struct Handle
	{
		Handle(const QString& audioFile);
		Sample sample;
		Sample::PlaybackState state;
		bool enabled = false;
	};
	Handle m_strongBeat, m_weakBeat;
	tick_t m_prevTicks = -1;
	std::atomic<bool> m_active = false;
	Song* m_song = nullptr;
};
} // namespace lmms

#endif // LMMS_METRONOME_H
