/*
 * SamplePlaybackState.cpp - Playback state for Sample
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

#include "SamplePlaybackState.h"
#include "lmms_basics.h"
#include "lmms_export.h"

#include <samplerate.h>
#include <stdexcept>

namespace lmms
{
    SamplePlaybackState::SamplePlaybackState(bool varyingPitch, int mode) :
        m_varyingPitch(varyingPitch),
        m_interpolationMode(mode)
    {
        int error = 0;
        if ((m_resamplingData = src_new(mode, DEFAULT_CHANNELS, &error)) == nullptr)
        {
            throw std::runtime_error{"Error when creating resample state: " + std::string{src_strerror(error)}};
        }
    }

    SamplePlaybackState::~SamplePlaybackState() noexcept
    {
        src_delete(m_resamplingData);
    }

    auto SamplePlaybackState::frameIndex() const -> f_cnt_t
    {
        return m_frameIndex;
    }

    auto SamplePlaybackState::varyingPitch() const -> bool
    {
        return m_varyingPitch;
    }

    auto SamplePlaybackState::isBackwards() const -> bool
    {
        return m_backwards;
    }

    auto SamplePlaybackState::interpolationMode() const -> int
    {
        return m_interpolationMode;
    }

    auto SamplePlaybackState::setFrameIndex(f_cnt_t index) -> void
    {
        m_frameIndex = index;
    }

    auto SamplePlaybackState::setVaryingPitch(bool varyingPitch) -> void
    {
        m_varyingPitch = varyingPitch;
    }

    auto SamplePlaybackState::setBackwards(bool backwards) -> void
    {
        m_backwards = backwards;
    }
} // namespace lmms
