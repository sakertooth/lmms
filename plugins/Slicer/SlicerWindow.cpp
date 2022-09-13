/*
 * SlicerWindow.cpp
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

#include "SlicerWindow.h"
#include <QVBoxLayout>

namespace lmms::gui 
{
    SlicerWindow::SlicerWindow(QWidget* parent, SlicerInstrument* instrument) : 
        QWidget(parent),
        m_slicerInstrument(dynamic_cast<SlicerInstrument*>(instrument)) 
    {
        setFixedSize(1000, 600);
        setWindowTitle(tr("Slicer"));

        m_samplePathLabel = new QLabel{this};
        m_samplePathLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
        m_samplePathLabel->setFixedSize(800, 25);

        m_openSampleButton = new QPushButton(tr("Open Sample"), this);

        auto openSampleGroupBox = new QGroupBox(this);
        auto openSampleLayout = new QHBoxLayout(this);
        openSampleLayout->addWidget(m_samplePathLabel, Qt::AlignHCenter);
        openSampleLayout->addWidget(m_openSampleButton, 0, Qt::AlignHCenter);
        openSampleGroupBox->setLayout(openSampleLayout);
        openSampleGroupBox->setFlat(true);
        openSampleGroupBox->move(1, 1);
        
        m_sampleWaveform = new SlicerWaveform(this);
        m_sampleWaveform->setFixedSize(1000, 375);
        m_sampleWaveform->move(0, 60);

        m_numSlicesLcd = new LcdSpinBox(3, this, tr("Slices"));
        m_numSlicesLcd->setLabel(tr("SLICES"));
        m_numSlicesLcd->setModel(&m_numSlicesLcdModel);

        m_incrementSlice = new QPushButton("+", this);
        m_incrementSlice->setFixedSize(16, 16);

        m_decrementSlice = new QPushButton("-", this);
        m_decrementSlice->setFixedSize(16, 16);
        
        m_sliceTypeBasic = new QRadioButton(tr("BASIC"), this);
        m_sliceTypeBasic->setChecked(true);
        m_sliceTypeOnsets = new QRadioButton(tr("ONSETS"), this);
        m_sliceButton = new QPushButton(tr("Slice"), this);

        auto slicingGroupBox = new QGroupBox(tr("Slicing"), this);
        auto slicingGroupBoxLayout = new QVBoxLayout();
        slicingGroupBoxLayout->addWidget(m_incrementSlice, 0, Qt::AlignHCenter);
        slicingGroupBoxLayout->addWidget(m_numSlicesLcd, 0, Qt::AlignHCenter);
        slicingGroupBoxLayout->addWidget(m_decrementSlice, 0, Qt::AlignHCenter);
        slicingGroupBoxLayout->addWidget(m_sliceButton, 0, Qt::AlignHCenter);
        slicingGroupBox->setLayout(slicingGroupBoxLayout);
        slicingGroupBox->move(20, 440);

        auto sliceTypeGroupBox = new QGroupBox(tr("Slice Type"), this);
        auto sliceTypeLayout = new QVBoxLayout();
        sliceTypeLayout->addWidget(m_sliceTypeBasic);
        sliceTypeLayout->addWidget(m_sliceTypeOnsets);
        sliceTypeGroupBox->setLayout(sliceTypeLayout);
        sliceTypeGroupBox->move(150, 440);

        connect(m_openSampleButton, &QPushButton::clicked, [this](){ m_slicerInstrument->loadSample(); });
        connect(m_slicerInstrument, &SlicerInstrument::sampleLoaded, [this](){ onSampleLoaded(); });
        connect(m_incrementSlice, &QPushButton::clicked, [this](){ m_numSlicesLcdModel.setValue(m_numSlicesLcdModel.value() + 1); });
        connect(m_decrementSlice, &QPushButton::clicked, [this](){ m_numSlicesLcdModel.setValue(m_numSlicesLcdModel.value() - 1); });
        connect(m_sliceButton, &QPushButton::clicked, [this](){ onSlice(); });
    }

    void SlicerWindow::onSampleLoaded()
    {
        m_samplePathLabel->setText(m_slicerInstrument->m_samplePath);
        m_sampleWaveform->loadSample(m_slicerInstrument->m_samples);
        m_sampleWaveform->update();
    }

    void SlicerWindow::onSlice() 
    {
        m_sampleWaveform->m_sliceLines.clear();

        const int sliceCount = m_numSlicesLcdModel.value();
        
        if (m_sliceTypeBasic->isChecked())
        {
           const int sliceSampleX = m_slicerInstrument->m_samples.size() / static_cast<float>(sliceCount + 1);

           for (int i = 0; i < sliceCount; ++i) 
           {
            m_sampleWaveform->addSlice(sliceSampleX + i * sliceSampleX);
           }
           update(); 
        }
        else if (m_sliceTypeOnsets->isChecked()) 
        {
            //Aubio
        }
    }
}