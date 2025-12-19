/*
 * AudioBuffer.cpp
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#include "AudioBuffer.h"

#include <QFile>
#include <optional>
#include <samplerate.h>
#include <sndfile.h>

#include "DrumSynth.h"

#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

namespace lmms {

namespace {
std::optional<AudioBuffer> decodeAudioFileSF(const QString& path)
{
	auto sndFile = static_cast<SNDFILE*>(nullptr);
	auto sfInfo = SF_INFO{};

	// TODO: Remove use of QFile
	auto file = QFile{path};
	if (!file.open(QIODevice::ReadOnly)) { throw std::runtime_error{"Failed to open audio file for reading"}; }

	sndFile = sf_open_fd(file.handle(), SFM_READ, &sfInfo, false);
	if (sf_error(sndFile) != 0) { throw std::runtime_error{"An error occurred when trying to read the audio file"}; }

	auto result = AudioBuffer(sfInfo.frames, sfInfo.channels, sfInfo.samplerate);
	sf_readf_float(sndFile, result.data(), result.numFrames());

	sf_close(sndFile);
	file.close();

	return result;
}

std::optional<AudioBuffer> decodeAudioFileDS(const QString& path)
{
	constexpr auto DefaultSampleRate = 44100;

	// Populated by DrumSynth::GetDSFileSamples
	int_sample_t* dataPtr = nullptr;

	auto ds = DrumSynth{};
	const auto frames = ds.GetDSFileSamples(path, dataPtr, 2, DefaultSampleRate);
	const auto data = std::unique_ptr<int_sample_t[]>{dataPtr}; // NOLINT, we have to use a C-style array here

	if (frames <= 0 || !data) { throw std::runtime_error{"Failed to load DrumSynth audio"}; }

	auto result = AudioBuffer(frames, 2, DefaultSampleRate);
	src_short_to_float_array(data.get(), &result.data()[0], frames * 2);

	return result;
}

std::optional<AudioBuffer> decodeAudioFileVorbis(const QString& path)
{
#ifdef LMMS_HAVE_OGGVORBIS
	static auto s_read = [](void* buffer, size_t size, size_t count, void* stream) -> size_t {
		auto file = static_cast<QFile*>(stream);
		return file->read(static_cast<char*>(buffer), size * count);
	};

	static auto s_seek = [](void* stream, ogg_int64_t offset, int whence) -> int {
		auto file = static_cast<QFile*>(stream);
		if (whence == SEEK_SET) { file->seek(offset); }
		else if (whence == SEEK_CUR) { file->seek(file->pos() + offset); }
		else if (whence == SEEK_END) { file->seek(file->size() + offset); }
		else
		{
			return -1;
		}
		return 0;
	};

	static auto s_close = [](void* stream) -> int {
		auto file = static_cast<QFile*>(stream);
		file->close();
		return 0;
	};

	static auto s_tell = [](void* stream) -> long {
		auto file = static_cast<QFile*>(stream);
		return file->pos();
	};

	static ov_callbacks s_callbacks = {s_read, s_seek, s_close, s_tell};

	// TODO: Remove use of QFile
	auto file = QFile{audioFile};
	if (!file.open(QIODevice::ReadOnly)) { throw std::runtime_error{"Failed to open audio file for reading"}; }

	auto vorbisFile = OggVorbis_File{};
	if (!file.open(QIODevice::ReadOnly)) { throw std::runtime_error{"Failed to open audio file for reading"}; }

	const auto vorbisInfo = ov_info(&vorbisFile, -1);
	if (vorbisInfo == nullptr) { throw std::runtime_error{"Failed to retrieve Vorbis data"}; }

	const auto numChannels = vorbisInfo->channels;
	const auto sampleRate = vorbisInfo->rate;
	const auto numSamples = ov_pcm_total(&vorbisFile, -1);
	if (vorbisInfo == nullptr) { throw std::runtime_error{"Failed to retrieve Vorbis data"}; }

	auto buffer = std::vector<float>(numSamples);
	auto output = static_cast<float**>(nullptr);

	auto totalSamplesRead = 0;
	while (true)
	{
		auto samplesRead = ov_read_float(&vorbisFile, &output, numSamples, 0);

		if (samplesRead < 0) { throw std::runtime_error{"Failed to read Vorbis data"}; }
		else if (samplesRead == 0) { break; }

		std::copy_n(*output, samplesRead, buffer.begin() + totalSamplesRead);
		totalSamplesRead += samplesRead;
	}

	auto result = std::vector<SampleFrame>(totalSamplesRead / numChannels);
	for (auto i = std::size_t{0}; i < result.size(); ++i)
	{
		if (numChannels == 1) { result[i] = {buffer[i], buffer[i]}; }
		else if (numChannels > 1) { result[i] = {buffer[i * numChannels], buffer[i * numChannels + 1]}; }
	}

	ov_clear(&vorbisFile);
	return SampleDecoder::Result{std::move(result), static_cast<int>(sampleRate)};
#else
	return std::nullopt;
#endif // LMMS_HAVE_OGGVORBIS
}
} // namespace

AudioBuffer::AudioBuffer(f_cnt_t numFrames, f_cnt_t numChannels, sample_rate_t sampleRate)
	: m_data(numFrames * numChannels)
	, m_sampleRate(sampleRate)
	, m_numChannels(numChannels)
	, m_numFrames(numFrames)
{
}

sample_t& AudioBuffer::sampleAt(f_cnt_t frameIndex, ch_cnt_t channelIndex)
{
	assert(frameIndex < m_numFrames);
	assert(channelIndex < m_numChannels);
	return m_data[frameIndex * m_numChannels + channelIndex];
}

AudioBuffer AudioBuffer::createFromFile(const QString& path)
{
	if (auto buffer = decodeAudioFileSF(path); buffer.has_value()) { return buffer.value(); }
	else if (auto buffer = decodeAudioFileDS(path); buffer.has_value()) { return buffer.value(); }
	else if (auto buffer = decodeAudioFileVorbis(path); buffer.has_value()) { return buffer.value(); }
	throw std::runtime_error{"Audio file is unsupported"};
}

QString AudioBuffer::toBase64() const
{
	// TODO: Replace with non-Qt equivalent
	const auto data = reinterpret_cast<const char*>(m_data.data());
	const auto size = static_cast<int>(m_data.size() * sizeof(float));
	const auto byteArray = QByteArray{data, size};
	return byteArray.toBase64();
}

} // namespace lmms
