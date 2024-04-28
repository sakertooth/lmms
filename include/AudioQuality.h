/*
 * AudioQuality.h - Stores audio quality settings used throughout the application
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

#ifndef LMMS_AUDIO_QUALITY_H
#define LMMS_AUDIO_QUALITY_H

#include <atomic>

namespace lmms {
class AudioQuality
{
public:
	enum class ResampleQuality
	{
		Linear,
		Fastest,
		Medium,
		Best,
		Count
	};

	static constexpr auto ResampleQualityCount = static_cast<std::size_t>(ResampleQuality::Count);
	static constexpr auto DefaultResampleQuality = ResampleQuality::Fastest;

	//! Return the resample quality currently in use
	static auto resampleQuality() -> ResampleQuality;

	//! Return the simple name for the given resample quality
	static auto resampleQualityName(ResampleQuality quality) -> const char*;

	//! Change the current resample quality to the given quality
	static void setResampleQuality(ResampleQuality quality);

	//! Returns the equivalent converter type used in libsamplerate for the given resample quality
	static int libSrcConverterType(ResampleQuality quality);

private:
	inline static std::atomic<ResampleQuality> s_resampleQuality = DefaultResampleQuality;
};

} // namespace lmms

#endif // LMMS_AUDIO_QUALITY_H
