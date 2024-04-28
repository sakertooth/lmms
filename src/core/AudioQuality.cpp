/*
 * AudioQuality.coo - Stores audio quality settings used throughout the application
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
#include "AudioQuality.h"
#include <samplerate.h>

namespace lmms {

auto AudioQuality::resampleQuality() -> ResampleQuality
{
	return s_resampleQuality.load(std::memory_order_relaxed);
}

void AudioQuality::setResampleQuality(ResampleQuality quality)
{
	s_resampleQuality.store(quality, std::memory_order_relaxed);
}

auto AudioQuality::resampleQualityName(ResampleQuality quality) -> const char*
{
	switch (quality)
	{
	case ResampleQuality::Linear:
		return "Linear";
	case ResampleQuality::Fastest:
		return "Fastest";
	case ResampleQuality::Medium:
		return "Medium";
	case ResampleQuality::Best:
		return "Best";
	default:
		return "";
	}
}

int AudioQuality::libSrcConverterType(ResampleQuality quality)
{
    switch (quality) {
	case ResampleQuality::Linear:
        return SRC_LINEAR;
	case ResampleQuality::Fastest:
        return SRC_SINC_FASTEST;
	case ResampleQuality::Medium:
        return SRC_SINC_MEDIUM_QUALITY;
	case ResampleQuality::Best:
        return SRC_SINC_BEST_QUALITY;
	default:
        return libSrcConverterType(DefaultResampleQuality);
	}
}
} // namespace lmms