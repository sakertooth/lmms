/*
 * SlicerInstrument.cpp - A sample slicer for LMMS
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

#include "ConfigManager.h"
#include "embed.h"
#include "InstrumentTrack.h"
#include "Plugin.h"
#include "plugin_export.h"
#include "SlicerInstrument.h"
#include "SlicerView.h"

#include <QFileDialog>

namespace lmms 
{
    extern "C" 
    {
        Plugin::Descriptor PLUGIN_EXPORT slicer_plugin_descriptor
            = {LMMS_STRINGIFY(PLUGIN_NAME), "Slicer", QT_TRANSLATE_NOOP("pluginBrowser", "A sample slicer for LMMS"),
                "saker <sakertooth@gmail.com>", 0x0100, Plugin::Instrument, new PluginPixmapLoader("logo"), nullptr, nullptr};
    }

    extern "C" 
    {
        Plugin* PLUGIN_EXPORT lmms_plugin_main(Model*, void* data)
        {
            return new SlicerInstrument(static_cast<InstrumentTrack*>(data));
        }
    }

    SlicerInstrument::SlicerInstrument(InstrumentTrack* track) : Instrument(track, &slicer_plugin_descriptor)
    {

    }

    void SlicerInstrument::playNote(NotePlayHandle *noteHandle, sampleFrame* buffer) 
    {

    }

    void SlicerInstrument::saveSettings(QDomDocument& doc, QDomElement& parent) 
    {

    }

    void SlicerInstrument::loadSettings(const QDomElement& storedData) 
    {

    }

    void SlicerInstrument::loadSample() 
    {
        m_samplePath = openSample();
        emit sampleLoaded(m_samplePath);
    }

    QString SlicerInstrument::openSample() 
    {
        QString sample = QFileDialog::getOpenFileName(nullptr, 
            tr("Open sample"), ConfigManager::inst()->userSamplesDir(), 
            tr("Samples (*.wav *.ogg)"));
            
        return sample.isNull() ? "" : sample;
    }

    QString SlicerInstrument::nodeName() const 
    {
        return slicer_plugin_descriptor.name;
    }

    gui::PluginView* SlicerInstrument::instantiateView(QWidget* parent) 
    {
        return new gui::SlicerView(this, parent);
    }

} // namespace lmms