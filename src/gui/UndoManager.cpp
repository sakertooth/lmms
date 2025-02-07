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

#include "AudioEngine.h"
#include "Clip.h"
#include "ClipView.h"
#include "Engine.h"
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
	const auto guard = Engine::audioEngine()->requestChangesGuard();
	m_track->removeClip(m_clip);
}

void CreateClipCommand::redo()
{
	const auto guard = Engine::audioEngine()->requestChangesGuard();

	if (m_clip != nullptr)
	{
		m_track->addClip(m_clip);
		return;
	}

	m_clip = m_track->createClip(m_pos);
}

Clip* CreateClipCommand::clip()
{
	return m_clip;
}

RemoveClipsCommand::RemoveClipsCommand(std::vector<Clip*> clips)
	: m_clips(std::move(clips))
{
}

void RemoveClipsCommand::undo()
{
	const auto guard = Engine::audioEngine()->requestChangesGuard();

	for (auto& clip : m_clips)
	{
		if (clip->getTrack() == nullptr) { continue; }
		clip->getTrack()->addClip(clip);
	}
}

void RemoveClipsCommand::redo()
{
	const auto guard = Engine::audioEngine()->requestChangesGuard();

	for (auto& clip : m_clips)
	{
		if (clip->getTrack() == nullptr) { continue; }
		clip->getTrack()->removeClip(clip);
	}
}

} // namespace lmms::gui