/*
 * Sample.h - State for container-class SampleBuffer
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

#ifndef SAMPLE_H
#define SAMPLE_H

#include <samplerate.h>
#include "lmms_basics.h"
#include "lmms_export.h"

namespace lmms
{
    class LMMS_EXPORT Sample
    {
    public:
        struct PlayMarkers
        {
            f_cnt_t startFrame;
            f_cnt_t endFrame;
            f_cnt_t loopStartFrame;
            f_cnt_t loopEndFrame;
        };

        Sample(bool varyingPitch = false, int mode = SRC_LINEAR);
        ~Sample() noexcept;

        auto startFrame() const -> f_cnt_t;
        auto endFrame() const -> f_cnt_t;
        auto loopStartFrame() const -> f_cnt_t;
        auto loopEndFrame() const -> f_cnt_t;
        auto frameIndex() const -> f_cnt_t;
        auto varyingPitch() const -> bool;
        auto isBackwards() const -> bool;
        auto interpolationMode() const -> int;

        auto setStartFrame(f_cnt_t frame) -> void;
        auto setEndFrame(f_cnt_t frame) -> void;
        auto setLoopStartFrame(f_cnt_t frame) -> void;
        auto setLoopEndFrame(f_cnt_t frame) -> void;
        auto setAllPointFrames(PlayMarkers playMarkers) -> void;
        auto setFrameIndex(f_cnt_t index) -> void;
        auto setVaryingPitch(bool varyingPitch) -> void;
        auto setBackwards(bool backwards) -> void;
    private:
        PlayMarkers m_playMarkers = {0, 0, 0, 0};
        f_cnt_t m_frameIndex = 0;
        bool m_varyingPitch = false;
        bool m_isBackwards = false;
        SRC_STATE* m_resamplingData = nullptr;
        int m_interpolationMode = SRC_LINEAR;
        friend class SampleBuffer;
    };
} // namespace lmms

#endif
