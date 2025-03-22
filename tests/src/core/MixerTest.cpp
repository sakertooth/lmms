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

#include "AudioEngine.h"
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
        constexpr auto sender = 1;
        constexpr auto receiver = 2;

		setup(2);
		const auto route = lmms::Engine::mixer()->createChannelSend(sender, receiver, 1.f);

		QCOMPARE(route->sender()->index(), sender);
		QCOMPARE(route->receiver()->index(), receiver);

		QCOMPARE(lmms::Engine::mixer()->containsSender(sender, route), true);
		QCOMPARE(lmms::Engine::mixer()->containsReceiver(receiver, route), true);
	}

	void testCreatedRouteHasDefaultAmount()
	{
        constexpr auto sender = 1;
        constexpr auto receiver = 2;

		setup(2);
		const auto route = lmms::Engine::mixer()->createChannelSend(sender, receiver, 1.f);

		QCOMPARE(route->amount()->value(), 1.f);
	}

	void testCreatedRouteDoesNotCreateInfiniteLoop()
	{
		setup(3);

		lmms::Engine::mixer()->createChannelSend(1, 2); 
		lmms::Engine::mixer()->createChannelSend(2, 3);
		const auto infiniteLoopRoute = lmms::Engine::mixer()->createChannelSend(3, 1);

		QCOMPARE(infiniteLoopRoute, nullptr);
	}

	void testDeletedRouteDoesNotExist()
	{
        constexpr auto sender = 1;
        constexpr auto receiver = 2;

        setup(2);
		const auto route = lmms::Engine::mixer()->createChannelSend(sender, receiver, 1.f);

		lmms::Engine::mixer()->deleteChannelSend(route);

		QCOMPARE(lmms::Engine::mixer()->containsSender(sender, route), false);
		QCOMPARE(lmms::Engine::mixer()->containsReceiver(receiver, route), false);
	}

	void testMuteSilencesAudioOutput()
	{
		setup(2);

		// TODO: Pass in frames to mixer channel's rather than have it coupled to the audio engine
		const auto framesPerPeriod = lmms::Engine::audioEngine()->framesPerPeriod();

		lmms::Engine::mixer()->createChannelSend(1, 2);
		lmms::Engine::mixer()->deleteChannelSend(1, 0);
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(1);

		std::fill_n(channelOne->m_buffer, framesPerPeriod, lmms::SampleFrame{1.f, 1.f});
		channelOne->m_muteModel.setValue(true);
		channelOne->process();

		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(2);
		const auto muted = std::all_of(channelTwo->m_buffer, channelTwo->m_buffer + framesPerPeriod,
			[](const auto& frame) { return frame.left() == 0.f && frame.right() == 0.f; });

		QCOMPARE(muted, true);
	}

	void testUnmuteRestoresAudioOutput()
	{
		setup(2);

		const auto framesPerPeriod = lmms::Engine::audioEngine()->framesPerPeriod();

		lmms::Engine::mixer()->createChannelSend(1, 2);
		lmms::Engine::mixer()->deleteChannelSend(1, 0);
		const auto channelOne = lmms::Engine::mixer()->mixerChannel(1);

		std::fill_n(channelOne->m_buffer, framesPerPeriod, lmms::SampleFrame{1.f, 1.f});
		channelOne->process();

		const auto channelTwo = lmms::Engine::mixer()->mixerChannel(2);
		const auto muted = std::all_of(channelTwo->m_buffer, channelTwo->m_buffer + framesPerPeriod,
			[](const auto& frame) { return frame.left() == 1.f && frame.right() == 1.f; });

		QCOMPARE(muted, false);
	}

	void testSoloMuteOtherChannelsButThoseRouted() {}

	void testUnsoloRestoresMuteState() {}

	void testAddedEffectExists() {}

	void testRemovedEffectDoesNotExist() {}
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
