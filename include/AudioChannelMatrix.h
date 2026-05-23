#ifndef LMMS_AUDIO_CHANNEL_MATRIX_H
#define LMMS_AUDIO_CHANNEL_MATRIX_H

#include <array>
#include <optional>
#include <variant>

#include "LmmsTypes.h"

namespace lmms {

template <ch_cnt_t NumSrcChannels, ch_cnt_t NumDstChannels>
using AudioChannelMatrix = std::array<std::array<float, NumDstChannels>, NumSrcChannels>;

using DynamicChannelMatrix = std::variant<AudioChannelMatrix<1, 2>, AudioChannelMatrix<2, 1>, AudioChannelMatrix<1, 1>,
	AudioChannelMatrix<2, 2>>;

static constexpr auto MonoToMonoMatrix = AudioChannelMatrix<1, 1>{{{1.f}}};
static constexpr auto MonoToSteroMatrix = AudioChannelMatrix<1, 2>{{{0.5, 0.5}}};
static constexpr auto StereoToMonoMatrix = AudioChannelMatrix<2, 1>{{{0.5}, {0.5}}};
static constexpr auto StereoToStereoMatrix = AudioChannelMatrix<2, 2>{{{1.f}, {1.f}}};

auto fetchChannelMatrix(ch_cnt_t srcChannels, ch_cnt_t dstChannels) -> std::optional<DynamicChannelMatrix>
{
	if (srcChannels == 1 && dstChannels == 2) { return MonoToSteroMatrix; }
	else if (srcChannels == 2 && dstChannels == 1) { return StereoToMonoMatrix; }
	else if (srcChannels == 1 && dstChannels == 1) { return MonoToMonoMatrix; }
	else if (srcChannels == 2 && dstChannels == 2) { return StereoToStereoMatrix; }
	else
	{
		return std::nullopt;
	}
}

} // namespace lmms

#endif // LMMS_AUDIO_CHANNEL_MATRIX_H