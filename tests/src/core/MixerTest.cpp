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
	void initTestCase() { lmms::Engine::init(true); }

	void cleanupTestCase() { lmms::Engine::destroy(); }

	void testHasMasterChannel() { QCOMPARE(lmms::Engine::mixer()->containsChannel(0), true); }

    void testCreateChannelExists()
	{
		const auto index = lmms::Engine::mixer()->createChannel();

        QCOMPARE(lmms::Engine::mixer()->containsChannel(index), true);
	}

	void testCreateChannelCorrectDefaults()
	{
		const auto index = lmms::Engine::mixer()->createChannel();
		const auto channel = lmms::Engine::mixer()->mixerChannel(index);

        QCOMPARE(channel->m_name, tr("Channel %1").arg(index));
        QCOMPARE(channel->m_volumeModel.value(), 1.f);
        QCOMPARE(channel->m_muteModel.value(), false);
        QCOMPARE(channel->m_soloModel.value(), false);
        QCOMPARE(channel->m_fxChain.empty(), true);
        QCOMPARE(channel->color().has_value(), false);
	}

	void testDeleteChannelDoesNotExist()
	{
		const auto index = lmms::Engine::mixer()->createChannel();
		lmms::Engine::mixer()->deleteChannel(index);
		QCOMPARE(lmms::Engine::mixer()->containsChannel(index), false);
	}

	void testMoveChannelLeftSwappedChannelIndices()
	{
		const auto indexOne = lmms::Engine::mixer()->createChannel();
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

		const auto indexTwo = lmms::Engine::mixer()->createChannel();
		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

		lmms::Engine::mixer()->moveChannelLeft(indexTwo);
		QCOMPARE(channelOne->index(), indexTwo);
		QCOMPARE(channelTwo->index(), indexOne);
	}

	void testMoveChannelRightSwappedChannelIndicies()
	{
		const auto indexOne = lmms::Engine::mixer()->createChannel();
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

		const auto indexTwo = lmms::Engine::mixer()->createChannel();
		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

		lmms::Engine::mixer()->moveChannelRight(indexOne);
		QCOMPARE(channelOne->index(), indexTwo);
		QCOMPARE(channelTwo->index(), indexOne);
	}

	void testCreateRouteCorrectSenderReceiver()
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

	void testRemoveRouteDoesNotExist()
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

	void testSoloChannelMuteOtherChannels()
	{
		const auto indexOne = lmms::Engine::mixer()->createChannel();
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(indexOne);

		const auto indexTwo = lmms::Engine::mixer()->createChannel();
		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(indexTwo);

		channelOne->m_soloModel.setValue(true);
		lmms::Engine::mixer()->toggledSolo();

		QCOMPARE(channelTwo->m_muteModel.value(), true);
	}
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
