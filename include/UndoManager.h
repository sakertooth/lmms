/*
 * UndoManager.h
 *
 * Copyright (c) 2024 Sotonye Atemie <sakertooth@gmail.com>
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

#ifndef LMMS_UNDO_MANAGER_H
#define LMMS_UNDO_MANAGER_H

#include <QObject>
#include <QUndoCommand>

#include "TimePos.h"

class QUndoStack;

namespace lmms {
class Track;
class Clip;
}

namespace lmms::gui {

class ClipView;

class LMMS_EXPORT UndoManager : public QObject
{
	Q_OBJECT
public:
	static UndoManager* instance();
	void commit(QUndoCommand* cmd);
	const QUndoStack* undoStack() const { return m_undoStack; }

signals:
	void commandCommitted(const QUndoCommand* cmd);

private:
	QUndoStack* m_undoStack = new QUndoStack(this);
};

class CreateClipCommand : public QUndoCommand
{
public:
	CreateClipCommand(Track* track, TimePos pos);
	void undo() override;
	void redo() override;
	auto clip() -> Clip*;

private:
	Track* m_track = nullptr;
	Clip* m_clip = nullptr;
	TimePos m_pos;
};

class RemoveClipsCommand : public QUndoCommand
{
public:
	RemoveClipsCommand(std::vector<Clip*> clips);
	void undo() override;
	void redo() override;

private:
	std::vector<Clip*> m_clips;
};

} // namespace lmms::gui

#endif // LMMS_UNDO_MANAGER_H