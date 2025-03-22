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
private:
	template <typename... Route> void setup(std::size_t numChannels, Route&&... routes)
	{
		for (auto i = std::size_t{0}; i < numChannels; ++i)
		{
			lmms::Engine::mixer()->createChannel();
		}

		(lmms::Engine::mixer()->createChannelSend(
			 std::forward<Route>(routes).first, std::forward<Route>(routes).second, 1.f),
			...);
	}

private slots:
	void initTestCase() { lmms::Engine::init(true); }

	void cleanupTestCase() { lmms::Engine::destroy(); }

	void cleanup() { lmms::Engine::mixer()->clear(); }

	void testHasMasterChannel() { QCOMPARE(lmms::Engine::mixer()->containsChannel(0), true); }

	void testCreatedChannelExists()
	{
		const auto index = lmms::Engine::mixer()->createChannel();

		QCOMPARE(lmms::Engine::mixer()->containsChannel(index), true);
	}

	void testCreatedChannelHasCorrectDefaults()
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

	void testDeletedChannelDoesNotExist()
	{
		const auto index = lmms::Engine::mixer()->createChannel();

		lmms::Engine::mixer()->deleteChannel(index);

		QCOMPARE(lmms::Engine::mixer()->containsChannel(index), false);
	}

	void testMovingChannelLeftSwapsChannelIndicies()
	{
		setup(2);
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(1);
		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(2);

		lmms::Engine::mixer()->moveChannelLeft(2);

		QCOMPARE(channelOne->index(), 2);
		QCOMPARE(channelTwo->index(), 1);
	}

	void testMovingChannelRightSwapsChannelIndicies()
	{
		setup(2);
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(1);
		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(2);

		lmms::Engine::mixer()->moveChannelRight(1);

		QCOMPARE(channelOne->index(), 2);
		QCOMPARE(channelTwo->index(), 1);
	}

	void testCreatedRouteHasCorrectSenderReceiver()
	{
		setup(2, std::make_pair(0, 1));
		const auto route = lmms::Engine::mixer()->createChannelSend(0, 1, 1.f);

		QCOMPARE(route->sender()->index(), 0);
		QCOMPARE(route->receiver()->index(), 1);

		QCOMPARE(lmms::Engine::mixer()->containsSender(0, route), true);
		QCOMPARE(lmms::Engine::mixer()->containsReceiver(1, route), true);
	}

	void testCreatedRouteHasDefaultAmount()
	{
		setup(2, std::make_pair(0, 1));
		const auto route = lmms::Engine::mixer()->createChannelSend(0, 1, 1.f);

		QCOMPARE(route->amount()->value(), 1.f);
	}

	void testCreatedRouteDoesNotCreateInfiniteLoop() {}

	void testDeletedRouteDoesNotExist()
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

	void testMuteSilencesAudioOutput() {}

	void testUnmuteRestoresAudioOutput() {}

	void testSoloMuteOtherChannelsButThoseRouted() {}

	void testUnsoloRestoresMuteState() {}

	void testAddedEffectExists() {}

	void testRemovedEffectDoesNotExist() {}
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
