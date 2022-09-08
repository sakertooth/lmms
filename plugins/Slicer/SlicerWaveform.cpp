/*
 * SlicerWaveform.cpp
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

#include "SlicerWaveform.h"
#include <QPainter>
#include <iostream>

namespace lmms::gui
{
    SlicerWaveform::SlicerWaveform(QWidget* parent) : QWidget(parent) {}

    void SlicerWaveform::loadSample(const std::vector<float>& samples) 
    {
        m_waveform.clear();

        const int samplesPerPixel = samples.size() / rect().width();
        for (int currentPixel = 0; currentPixel < rect().width(); ++currentPixel) 
        {
            const int startingFrame = currentPixel * samplesPerPixel;
            const auto [minSample, maxSample] = std::minmax_element(samples.begin() + startingFrame, samples.begin() + startingFrame + samplesPerPixel);
            
            const QPointF centerPoint{static_cast<float>(currentPixel), rect().height() / 2.0f};
            const QPointF minPoint{centerPoint.x(), centerPoint.y() + std::abs(*minSample) * (rect().height() / 2.0f)};
            const QPointF maxPoint{centerPoint.x(), centerPoint.y() - std::abs(*maxSample) * (rect().height() / 2.0f)};

            m_waveform.emplace_back(minPoint, maxPoint);
        }
    }

    void SlicerWaveform::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::green);
        painter.drawLines(m_waveform.data(), m_waveform.size());
    }
}