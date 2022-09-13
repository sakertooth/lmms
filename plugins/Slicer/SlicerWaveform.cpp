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

        m_samples = &samples;
    }

    void SlicerWaveform::addSlice(int sampleIdx)
    {
        if (sampleIdx >= static_cast<int>(m_samples->size()) || sampleIdx < 0)
        {
            std::cerr << "(Slicer) Invalid sample index\n";
            return;
        }

        float sliceLineX = sampleIdx / (m_samples->size() / static_cast<float>(rect().width()));
        QLineF sliceLine(sliceLineX, rect().y(), sliceLineX, rect().y() + rect().height());
        m_sliceLines[sampleIdx] = sliceLine;
    }

    void SlicerWaveform::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor{"#353333"});
        painter.setPen(QColor{"#00b530"});
        painter.drawLines(m_waveform.data(), m_waveform.size());

        painter.setPen(QColor{"#c5c3c3"});
        for (const auto& slice : m_sliceLines) 
        {
            const auto& [_, sliceLine] = slice;
            painter.drawLine(sliceLine);
        }
    }
}