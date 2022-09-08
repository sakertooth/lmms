/*
 * SlicerView.cpp - A sample slicer for LMMS
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

#include "SlicerView.h"
#include "SlicerInstrument.h"
#include "embed.h"

namespace lmms::gui 
{
    SlicerView::SlicerView(Instrument* instrument, QWidget* parent) 
    : InstrumentViewFixedSize(instrument, parent),
      m_slicerInstrument(dynamic_cast<SlicerInstrument*>(instrument))
    {
        m_samplePathLabel = new QLabel{this};
        m_samplePathLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
        m_samplePathLabel->setFixedSize(650, 25);
        m_samplePathLabel->move(5, 5);
        m_samplePathLabel->setToolTip(tr("Sample path"));

        m_openSampleButton = new PixmapButton{this};
        m_openSampleButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("select_sample"));
	    m_openSampleButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("select_sample"));
        m_openSampleButton->setFixedSize(32, 32);
        m_openSampleButton->move(665, 0);
        m_openSampleButton->setToolTip(tr("Open sample"));

        m_slicerWaveform = new SlicerWaveform(this);
        m_slicerWaveform->setFixedSize(700, 125);
        m_slicerWaveform->move(0, 40);
        m_slicerWaveform->setToolTip(tr("Sample waveform"));
        
        connect(m_openSampleButton, &PixmapButton::clicked, [this]() { m_slicerInstrument->loadSample(); });
        connect(m_slicerInstrument, &SlicerInstrument::sampleLoaded, [this](){ onSampleLoaded(); });
    }

    QSize SlicerView::sizeHint() const { return QSize{700, 250}; }
    
    void SlicerView::onSampleLoaded() 
    {
        m_samplePathLabel->setText(m_slicerInstrument->m_samplePath);
        m_slicerWaveform->loadSample(m_slicerInstrument->m_samples);
        m_slicerWaveform->update();
    }
}