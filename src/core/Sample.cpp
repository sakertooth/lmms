/*
 * Sample.cpp - State for container-class SampleBuffer
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

#include "Sample.h"
#include <iostream>

namespace lmms
{
    auto Sample::startFrame() const -> f_cnt_t
    {
        return m_playMarkers.startFrame;
    }

    auto Sample::endFrame() const -> f_cnt_t
    {
        return m_playMarkers.endFrame;
    }

    auto Sample::loopStartFrame() const -> f_cnt_t
    {
        return m_playMarkers.loopStartFrame;
    }

    auto Sample::loopEndFrame() const -> f_cnt_t
    {
        return m_playMarkers.loopEndFrame;
    }


    auto Sample::setStartFrame(f_cnt_t frame) -> void
    {
        m_playMarkers.startFrame = frame;
    }

    auto Sample::setEndFrame(f_cnt_t frame) -> void
    {
        m_playMarkers.endFrame = frame;
    }

    auto Sample::setLoopStartFrame(f_cnt_t frame) -> void
    {
        m_playMarkers.loopStartFrame = frame;
    }

    auto Sample::setLoopEndFrame(f_cnt_t frame) -> void
    {
        m_playMarkers.loopEndFrame = frame;
    }

    auto Sample::setAllPointFrames(PlayMarkers playMarkers) -> void
    {
        m_playMarkers = playMarkers;
    }

    Sample::PlaybackState::PlaybackState(bool varyingPitch, int mode) :
        m_varyingPitch(varyingPitch),
        m_interpolationMode(mode)
    {
        int error = 0;
        if ((m_resamplingData = src_new(mode, DEFAULT_CHANNELS, &error)) == nullptr)
        {
            std::cerr << "Error when creating resample state: " << src_strerror(error) << '\n';
        }
    }

    Sample::PlaybackState::~PlaybackState() noexcept
    {
        src_delete(m_resamplingData);
    }

    auto Sample::PlaybackState::frameIndex() const -> f_cnt_t
    {
        return m_frameIndex;
    }

    auto Sample::PlaybackState::varyingPitch() const -> bool
    {
        return m_varyingPitch;
    }

    auto Sample::PlaybackState::isBackwards() const -> bool
    {
        return m_backwards;
    }

    auto Sample::PlaybackState::interpolationMode() const -> int
    {
        return m_interpolationMode;
    }

    auto Sample::PlaybackState::setFrameIndex(f_cnt_t index) -> void
    {
        m_frameIndex = index;
    }

    auto Sample::PlaybackState::setVaryingPitch(bool varyingPitch) -> void
    {
        m_varyingPitch = varyingPitch;
    }

    auto Sample::PlaybackState::setBackwards(bool backwards) -> void
    {
        m_backwards = backwards;
    }

} // namespace lmms
