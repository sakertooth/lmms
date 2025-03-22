/*
 * MixerViewTest.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <satemiej@gmail.com>
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

#include "MixerView.h"
#include <QObject>
#include <QtTest/QtTest>

#include "Engine.h"
#include "GuiApplication.h"
#include "Mixer.h"

class MixerViewTest : public QObject
{
	Q_OBJECT
private:
    lmms::Mixer* mixer() const { return lmms::Engine::mixer(); }

private slots:
	void initTestCase() { lmms::Engine::init(true);  }

	void cleanupTestCase() { lmms::Engine::destroy(); }

	void testClickOnChannelSelectsAsCurrent()
    {
        auto mixerView = lmms::gui::MixerView{mixer(), nullptr};
        mixerView.addNewChannel();

        auto newChannelView = mixerView.findChild<lmms::gui::MixerChannelView*>("mixerChannelView1");
        QTest::mouseClick(newChannelView, Qt::LeftButton);

        QCOMPARE(mixerView.currentMixerChannel(), newChannelView);
    }

	void testClickAddChannelButtonAddsChannel() {}

	void testClickRemoveChannelActionRemovesChannel() {}

	void testClickMuteButtonWhenUnmutedMutesChannel() {}

	void testClickMuteButtonWhenMutedUnmutesChannel() {}

	void testClickSoloButtonWhenNotSoloedSolosChannel() {}

	void testClickSoloButtonWhenSoloedUnsolosChannel() {}

	void testClickMoveChannelLeftActionMovesChannelLeft() {}

	void testClickMoveChannelRightActionMovesChannelRight() {}

	void testRemoveUnusedChannelsWorks() {}
};

QTEST_MAIN(MixerViewTest)
#include "MixerViewTest.moc"
