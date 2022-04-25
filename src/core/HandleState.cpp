/*
 * HandleState.cpp - HandleState
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

#include "HandleState.h"
#include <QDebug>

HandleState::HandleState(bool varyingPitch, int interpolationMode) :
	m_frameIndex(0),
	m_varyingPitch(varyingPitch),
	m_isBackwards(false)
{
	int error;
	m_interpolationMode = interpolationMode;

	if ((m_resamplingData = src_new(interpolationMode, DEFAULT_CHANNELS, &error)) == nullptr)
	{
		#ifdef DEBUG_LMMS
			qDebug("Error: src_new() failed in SampleBuffer.cpp!\n");
		#endif
	}
}

HandleState::~HandleState() 
{
	src_delete(m_resamplingData);
}

const f_cnt_t HandleState::frameIndex() const 
{
	return m_frameIndex;
}

void HandleState::setFrameIndex(f_cnt_t index) 
{
	m_frameIndex = index;
}

bool HandleState::isBackwards() const 
{
	return m_isBackwards;
}

void HandleState::setBackwards(bool backwards) 
{
	m_isBackwards = backwards;
}

int HandleState::interpolationMode() const 
{
	return m_interpolationMode;
}
