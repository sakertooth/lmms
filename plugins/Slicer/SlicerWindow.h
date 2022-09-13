/*
 * SlicerWindow.h
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
#include "SlicerInstrument.h"
#include "LcdSpinBox.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>

namespace lmms::gui
{
    class SlicerWindow : public QWidget
    {
    public:
        SlicerWindow(QWidget* parent, SlicerInstrument* slicerInstrument);
        void onSampleLoaded();
        
    private:
        QLabel* m_samplePathLabel;
        QPushButton* m_openSampleButton;
        SlicerWaveform* m_sampleWaveform;
        LcdSpinBox* m_numSlicesLcd;
        IntModel m_numSlicesLcdModel{0, 0, 128};
        QPushButton* m_sliceButton;
        QPushButton* m_incrementSlice;
        QPushButton* m_decrementSlice;
        QRadioButton* m_sliceTypeBasic;
        QRadioButton* m_sliceTypeOnsets;
        SlicerInstrument* m_slicerInstrument;
    };
}