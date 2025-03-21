/*
 * MathTest.cpp
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

#include "Mixer.h"
#include <QObject>
#include <QtTest/QtTest>

#include "Engine.h"

class MixerTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		lmms::Engine::init(true);
	}

    void cleanupTestCase()
    {
        lmms::Engine::destroy();
    }

    void init()
    {
        lmms::Engine::mixer()->clear();
    }

    void cleanup()
    {
        lmms::Engine::mixer()->clear();
    }

    void testHasOneChannelInitially()
    {
        QCOMPARE(lmms::Engine::mixer()->numChannels(), 1);
    }

    void testMasterChannelCreatedInitially()
    {
        QCOMPARE(lmms::Engine::mixer()->containsChannel(0), true);
    }

	void testCanAddChannel() {
        const auto index = lmms::Engine::mixer()->createChannel();
        QCOMPARE(index, 1);
    }

	void testCanRemoveChannel() {
        const auto indexOne = lmms::Engine::mixer()->createChannel();
        lmms::Engine::mixer()->deleteChannel(indexOne);
        QCOMPARE(lmms::Engine::mixer()->containsChannel(indexOne), false);
    }

    void testCanMoveChannelLeft() {
        const auto indexOne = lmms::Engine::mixer()->createChannel();
        const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

        const auto indexTwo = lmms::Engine::mixer()->createChannel();
        const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

        lmms::Engine::mixer()->moveChannelLeft(indexTwo);
        QCOMPARE(channelOne->index(), indexTwo);
        QCOMPARE(channelTwo->index(), indexOne);
    }

    void testCanMoveChannelRight() {
        const auto indexOne = lmms::Engine::mixer()->createChannel();
        const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

        const auto indexTwo = lmms::Engine::mixer()->createChannel();
        const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

        lmms::Engine::mixer()->moveChannelRight(indexOne);
        QCOMPARE(channelOne->index(), indexTwo);
        QCOMPARE(channelTwo->index(), indexOne);
    }

    void testCanCreateSendRoute() 
    {
        const auto indexOne = lmms::Engine::mixer()->createChannel();
        const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

        const auto indexTwo = lmms::Engine::mixer()->createChannel();
        const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

        const auto route = lmms::Engine::mixer()->createRoute(channelOne, channelTwo, 1.f);

        QCOMPARE(route->sender(), channelOne);
        QCOMPARE(route->receiver(), channelTwo);

        QCOMPARE(lmms::Engine::mixer()->containsSender(indexOne, route), true);
        QCOMPARE(lmms::Engine::mixer()->containsReceiver(indexTwo, route), true);
    }

    void testCanRemoveSendRoute()
    {
        const auto indexOne = lmms::Engine::mixer()->createChannel();
        const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

        const auto indexTwo = lmms::Engine::mixer()->createChannel();
        const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

        const auto route = lmms::Engine::mixer()->createRoute(channelOne, channelTwo, 1.f);

        lmms::Engine::mixer()->deleteChannelSend(route);

        QCOMPARE(lmms::Engine::mixer()->containsSender(indexOne, route), false);
        QCOMPARE(lmms::Engine::mixer()->containsReceiver(indexTwo, route), false);
    }

    void testCanSoloChannel()
    {
        
    }
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
