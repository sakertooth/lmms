/*
 * SlicerView.cpp
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
      m_showGuiButton = new QPushButton(tr("Show GUI"), this);
      m_showGuiButton->move(125, 125);
      m_slicerWindow = new SlicerWindow(nullptr, dynamic_cast<SlicerInstrument*>(instrument));        
      connect(m_showGuiButton, &QPushButton::clicked, [this](){ m_slicerWindow->show(); });
    }
}