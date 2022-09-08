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
    SlicerView::SlicerView(Instrument* instrument, QWidget* parent) : InstrumentViewFixedSize(instrument, parent)
    {
        m_samplePathLabel = new QLabel{this};
        m_samplePathLabel->setFrameStyle(QFrame::Box | QFrame::Plain);
        m_samplePathLabel->setFixedSize(650, 25);
        m_samplePathLabel->move(5, 5);

        m_openSampleButton = new PixmapButton{this};
        m_openSampleButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("select_sample"));
	    m_openSampleButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("select_sample"));
        m_openSampleButton->setFixedSize(32, 32);
        m_openSampleButton->move(665, 0);
        m_openSampleButton->setToolTip(tr("Open sample"));

        const auto& slicerInstrument = static_cast<SlicerInstrument*>(instrument);
        connect(m_openSampleButton, &PixmapButton::clicked, [slicerInstrument]() { slicerInstrument->loadSample(); });
        connect(slicerInstrument, &SlicerInstrument::sampleLoaded, [slicerInstrument, this](const QString& sample){ onSampleLoaded(sample); });
    }

    QSize SlicerView::sizeHint() const { return QSize{700, 250}; }
    
    void SlicerView::onSampleLoaded(const QString& sample) 
    {
        m_samplePathLabel->setText(sample);
    }

}