/*
 * TempoLcdSpinBox.h - LcdSpinBox but with a custom CaptionMenu for the Tempo widget
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

#include "LcdSpinBox.h"
#include "PluginView.h"

class TempoLcdSpinBox : public LcdSpinBox
{
	Q_OBJECT
public:
	TempoLcdSpinBox(int numDigits, QWidget* parent, const QString& name = QString());
	TempoLcdSpinBox(int numDigits, const QString& style, QWidget* parent, const QString& name = QString());

protected:
	void contextMenuEvent(QContextMenuEvent* _me) override;

private:
	void showTapTempoTool();
};