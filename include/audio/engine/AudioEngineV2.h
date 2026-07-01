/*
 * AudioEngine.h
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

#ifndef LMMS_AUDIO_ENGINE_V2
#define LMMS_AUDIO_ENGINE_V2

#include <memory>

#include "audio/engine/AudioTrack.h"
#include "audio/engine/AudioTrackEvents.h"
#include "audio/engine/AudioTrackRoute.h"

namespace lmms {
class AudioEngineV2
{
public:
    void render(InterleavedBufferView<float> dst);
    void render(PlanarBufferView<float> dst);

	auto addTrack(std::unique_ptr<AudioTrack> track) -> int;
	void removeTrack(int id);

    void addRoute(int fromTrackID, int toTrackID);
    void removeRoute(int fromTrackID, int toTrackID);

    void submitTrackEvent(AudioTrackEvent event);

private:
    struct AddTrackCommand
    {
    };

    struct RemoveTrackCommand
    {
    };

    struct AddRouteCommand
    {
    };

    struct RemoveRouteCommand
    {
    };

    using Message = std::variant<
        AudioTrackEvent,
        AddTrackCommand,
        RemoveTrackCommand,
        AddRouteCommand,
        RemoveRouteCommand>;

	ArrayVector<std::unique_ptr<AudioTrack>, 1024> m_audioTracks;
	ArrayVector<AudioBuffer, 1024> m_audioTrackBuses;
    ArrayVector<AudioTrackRoute, 8192> m_audioTrackRoutes;
};
} // namespace lmms

#endif // LMMS_AUDIO_ENGINE_V2