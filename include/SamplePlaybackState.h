/*
 * SamplePlaybackState.h - Playback state for Sample
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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
#ifndef SAMPLE_PLAYBACK_STATE_H
#define SAMPLE_PLAYBACK_STATE_H

#include <samplerate.h>
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms
{
    class LMMS_EXPORT SamplePlaybackState
    {
    public:
        SamplePlaybackState(bool varyingPitch = false, int mode = SRC_LINEAR);
        ~SamplePlaybackState() noexcept;

        auto frameIndex() const -> f_cnt_t;
        auto varyingPitch() const -> bool;
        auto isBackwards() const -> bool;
        auto interpolationMode() const -> int;

        auto setFrameIndex(f_cnt_t index) -> void;
        auto setVaryingPitch(bool varyingPitch) -> void;
        auto setBackwards(bool backwards) -> void;

    private:
        f_cnt_t m_frameIndex = 0;
        bool m_varyingPitch = false;
        bool m_backwards = false;
        SRC_STATE* m_resamplingData = nullptr;
        int m_interpolationMode = SRC_LINEAR;
        friend class Sample;
    };
} // namespace lmms

#endif
