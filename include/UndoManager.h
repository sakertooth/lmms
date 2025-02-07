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

#include <QObject>
#include <QUndoCommand>

class QUndoStack;

namespace lmms::gui {
class UndoManager : public QObject
{
public:
    static UndoManager* instance()
    {
        static const auto s_undoManager = new UndoManager();
        return s_undoManager;
    }

	template <typename Fn> void commit(Fn undo, Fn redo)
	{
		class UndoCommand : public QUndoCommand
		{
		public:
			UndoCommand(Fn undo, Fn redo)
				: m_undo(std::move(undo))
				, m_redo(std::move(redo))
			{
			}

			void undo() override { m_undo(); }
			void redo() override { m_redo(); }

		private:
			Fn m_undo;
			Fn m_redo;
		};

		const auto cmd = new UndoCommand(std::move(undo), std::move(redo));
		m_undoStack->push(cmd);
        emit commandCommitted(cmd);
	}

    const QUndoStack* undoStack() const { return m_undoStack; }

signals:
    void commandCommitted(QUndoCommand* cmd);

private:
    QUndoStack* m_undoStack = new QUndoStack(this);
};

}