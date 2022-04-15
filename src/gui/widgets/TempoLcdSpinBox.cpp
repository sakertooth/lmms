/*
 * TempoLcdSpinBox.cpp - LcdSpinBox but with a custom CaptionMenu for the Tempo widget
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#include "TempoLcdSpinBox.h"

#include "CaptionMenu.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "TextFloat.h"

#include <functional>
#include <algorithm>

TempoLcdSpinBox::TempoLcdSpinBox(int numDigits, QWidget* parent, const QString& name) : 
	LcdSpinBox(numDigits, parent, name) {}

TempoLcdSpinBox::TempoLcdSpinBox(int numDigits, const QString& style, QWidget* parent, const QString& name) :
	LcdSpinBox(numDigits, style, parent, name) {}

void TempoLcdSpinBox::contextMenuEvent(QContextMenuEvent* _me) 
{
	CaptionMenu contextMenu(model()->displayName());
	contextMenu.addAction("Tap Tempo", std::bind(&TempoLcdSpinBox::showTapTempoTool, this));
	addDefaultActions(&contextMenu);
	contextMenu.exec(QCursor::pos());
}

void TempoLcdSpinBox::showTapTempoTool()
{
	auto tools = getGUI()->mainWindow()->tools();
	auto tapTempoPvIt = std::find_if(tools.begin(), tools.end(), [](auto& tool) 
	{
		return tool->model()->displayName() == "Tap Tempo";
	});

	if (tapTempoPvIt == tools.end()) 
	{
		TextFloat::displayMessage("The Tap Tempo Tool could not be found.");
		return;
	}

	auto tapTempoPv = *tapTempoPvIt;
	tapTempoPv->show();
	tapTempoPv->parentWidget()->show();
	tapTempoPv->setFocus();
}
