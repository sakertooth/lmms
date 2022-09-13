/*
 * SlicerWaveform.h
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#ifndef SLICER_WAVEFORM_H
#define SLICER_WAVEFORM_H

#include <QWidget>
#include <QRect>
#include <vector>
#include "lmms_basics.h"

namespace lmms::gui
{
    class SlicerWaveform : public QWidget
    {
    public:
        SlicerWaveform(QWidget* parent);
        void loadSample(const std::vector<float>& samples);
        void paintEvent(QPaintEvent* event) override;     
        void addSlice(int frameIdx);
    private:
        std::vector<QLineF> m_waveform;
        std::unordered_map<int, QLineF> m_sliceLines;
        const std::vector<float>* m_samples;

        friend class SlicerWindow;
    };
}

#endif