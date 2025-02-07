/*
 * UndoManager.cpp
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

#include "UndoManager.h"

#include "Clip.h"
#include "Track.h"

namespace lmms::gui {

UndoManager* UndoManager::instance()
{
	static const auto s_undoManager = new UndoManager();
	return s_undoManager;
}

void UndoManager::commit(QUndoCommand* cmd)
{
	m_undoStack->push(cmd);
	emit commandCommitted(cmd);
}

CreateClipCommand::CreateClipCommand(Track* track, TimePos pos)
	: m_track(track)
	, m_pos(pos)
{
}

void CreateClipCommand::undo()
{
    m_track->removeClip(m_clip);
    delete m_clip;
}

void CreateClipCommand::redo()
{
	m_clip = m_track->createClip(m_pos);
}

Clip* CreateClipCommand::createdClip()
{
	return m_clip;
}

} // namespace lmms::gui