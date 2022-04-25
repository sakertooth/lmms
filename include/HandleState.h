/*
 * HandleState.h - HandleState
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#ifndef HANDLE_STATE_H
#define HANDLE_STATE_H

#include <samplerate.h>
#include "lmms_basics.h"

class SampleBuffer;

class HandleState
{
public:
	HandleState(bool varyingPitch = false, int interpolationMode = SRC_LINEAR);
	virtual ~HandleState();

	const f_cnt_t frameIndex() const;
	void setFrameIndex(f_cnt_t index);

	bool isBackwards() const;
	void setBackwards(bool backwards);

	int interpolationMode() const;
private:
	f_cnt_t m_frameIndex;
	const bool m_varyingPitch;
	bool m_isBackwards;
	SRC_STATE *m_resamplingData;
	int m_interpolationMode;

	friend class SampleBuffer;
};
#endif