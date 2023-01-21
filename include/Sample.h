/*
 * Sample.h - State for container-class SampleBuffer
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

#ifndef SAMPLE_H
#define SAMPLE_H

#include "AudioEngine.h"
#include "Engine.h"
#include "Note.h"
#include "SampleBuffer.h"

#include <samplerate.h>
#include "lmms_basics.h"
#include "lmms_export.h"

#include <QPainter>
#include <QRect>

namespace lmms
{
    class LMMS_EXPORT Sample : public QObject
    {
        Q_OBJECT
    public:
        enum class LoopMode
        {
            LoopOff, LoopOn, LoopPingPong
        };

        class LMMS_EXPORT PlaybackState
        {
        public:
            PlaybackState(bool varyingPitch = false, int mode = SRC_LINEAR);
            ~PlaybackState() noexcept;
            
            auto frameIndex() const -> f_cnt_t;
            auto varyingPitch() const -> bool;
            auto isBackwards() const -> bool;
            auto interpolationMode() const -> int;

            auto setFrameIndex(f_cnt_t index) -> void;
            auto setVaryingPitch(bool varyingPitch) -> void;
            auto setBackwards(bool backwards) -> void;

        private:
            f_cnt_t m_frameIndex = 0;
            bool m_varyingPitch = false;
            bool m_backwards = false;
            SRC_STATE* m_resamplingData = nullptr;
            int m_interpolationMode = SRC_LINEAR;
            friend class Sample;
        };

        struct PlayMarkers
        {
            f_cnt_t startFrame;
            f_cnt_t endFrame;
            f_cnt_t loopStartFrame;
            f_cnt_t loopEndFrame;
        };

        Sample() = default;
        Sample(std::shared_ptr<SampleBuffer> buffer);

        static auto createFromAudioFile(const QString& audioFile) -> std::shared_ptr<Sample>;
        static auto createFromBase64(const QString& base64) -> std::shared_ptr<Sample>;

        auto play(sampleFrame* dst, PlaybackState* state, fpp_t frames, float freq, LoopMode loopMode = LoopMode::LoopOff) -> bool;

        //! TODO: Should be moved to its own QWidget
        auto visualize(QPainter & p, const QRect & dr, f_cnt_t fromFrame = 0, f_cnt_t toFrame = 0) -> void;

        auto calculateSampleLength() const -> int;
        
        static auto interpolationMargins() -> std::array<f_cnt_t, 5>&;
        auto buffer() const -> std::shared_ptr<const SampleBuffer>;
        auto startFrame() const -> f_cnt_t;
        auto endFrame() const -> f_cnt_t;
        auto loopStartFrame() const -> f_cnt_t;
        auto loopEndFrame() const -> f_cnt_t;
        auto amplification() const -> float;
        auto frequency() const -> float;
        auto reversed() const -> bool;

        auto setSampleBuffer(std::shared_ptr<SampleBuffer> buffer) -> void;
        auto setStartFrame(f_cnt_t frame) -> void;
        auto setEndFrame(f_cnt_t frame) -> void;
        auto setLoopStartFrame(f_cnt_t frame) -> void;
        auto setLoopEndFrame(f_cnt_t frame) -> void;
        auto setAllPointFrames(PlayMarkers playMarkers) -> void;
        auto setAmplification(float amplification) -> void;
        auto setFrequency(float frequency) -> void;
        auto setReversed(bool reversed) -> void;
    signals:
        void sampleUpdated();
    private:
        auto getSampleFragment(
            f_cnt_t index,
            f_cnt_t frames,
            LoopMode loopMode,
            bool* backwards,
            f_cnt_t loopStart,
            f_cnt_t loopEnd,
            f_cnt_t end) const -> std::vector<sampleFrame>;
        
        auto scaleMarkersBySampleRate() -> void;
        auto advance(f_cnt_t playFrame, f_cnt_t frames, LoopMode loopMode, Sample::PlaybackState* state) -> f_cnt_t;
        auto getLoopedIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const -> f_cnt_t;
        auto getPingPongIndex(f_cnt_t index, f_cnt_t startFrame, f_cnt_t endFrame) const -> f_cnt_t;
    private:
        std::shared_ptr<const SampleBuffer> m_buffer = std::make_shared<SampleBuffer>();
        PlayMarkers m_playMarkers = {0, 0, 0, 0};
        float m_amplification = 1.0f;
        float m_frequency = DefaultBaseFreq;
        bool m_reversed = false;
        sample_rate_t m_markerSampleRate = 0;
    };
} // namespace lmms

#endif
