/*
 * SampleBuffer.cpp - State for container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
    Sample::Sample(bool varyingPitch, int interpolationMode) :
        m_varyingPitch(varyingPitch),
        m_interpolationMode(interpolationMode)
    {
        int error = 0;
        if ((m_resamplingData = src_new(interpolationMode, DEFAULT_CHANNELS, &error)) == nullptr)
        {
            std::cerr << "Error: src_new() failed in sample_buffer.cpp!\n";
        }
    }

    Sample::~Sample()
    {
        src_delete(m_resamplingData);
    }
} // namespace lmms
