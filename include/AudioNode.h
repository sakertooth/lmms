/*
 * AudioNode.h
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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

#ifndef LMMS_AUDIO_NODE_H
#define LMMS_AUDIO_NODE_H

#include "lmms_basics.h"

namespace lmms {
class AudioNode
{
public:
	struct Buffer
	{
		const sampleFrame* dest;
		size_t size;
	};

	virtual ~AudioNode() = default;

	//! Push a buffer as input to this node.
	virtual void push(Buffer buffer) = 0;

	//! Run audio processing logic for the current audio period.
	//! Returns the result of the audio processing as a buffer.
	virtual auto pull() -> Buffer = 0;
};
} // namespace lmms

#endif // LMMS_AUDIO_NODE_H
