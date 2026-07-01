/*
 * AudioTrackEvents.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published     by the Free Software Foundation; either
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

#ifndef LMMS_AUDIO_TRACK_EVENTS_H
#define LMMS_AUDIO_TRACK_EVENTS_H

#include <span>
#include <variant>

#include "SampleFrame.h"

namespace lmms {

struct MidiNoteOnEvent
{
	int trackID;
    f_cnt_t globalOffset;
};

struct MidiNoteOffEvent
{
	int trackID;
	int voiceID;
    f_cnt_t globalOffset;
};

struct SampleOnEvent
{
	std::span<SampleFrame> buffer;
	int trackID;
    f_cnt_t globalOffset;
};

struct SampleOffEvent
{
	int trackID;
	int voiceID;
    f_cnt_t globalOffset;
};

struct ClearVoicesEvent
{
	int trackID;
};

using AudioTrackEvent = std::variant<
    MidiNoteOnEvent,
    MidiNoteOffEvent,
    SampleOnEvent,
    SampleOffEvent,
    ClearVoicesEvent>;

} // namespace lmms

#endif