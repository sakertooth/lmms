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
        Sample(bool varyingPitch = false, int interpolationMode = SRC_LINEAR);
        ~Sample();

        inline f_cnt_t frameIndex() const { return m_frameIndex; }
		inline bool isBackwards() const { return m_isBackwards; }
		inline int interpolationMode() const { return m_interpolationMode; }

		inline void setFrameIndex(f_cnt_t index) { m_frameIndex = index; }
		inline void setBackwards(bool backwards) { m_isBackwards = backwards; }
        inline void setInterpolationMode(int mode) { m_interpolationMode = mode; }
    private:
        f_cnt_t m_frameIndex = 0;
		const bool m_varyingPitch = false;
		bool m_isBackwards = false;
		SRC_STATE* m_resamplingData = nullptr;
		int m_interpolationMode = SRC_LINEAR;

		friend class SampleBuffer;
    };
} // namespace lmms

#endif
