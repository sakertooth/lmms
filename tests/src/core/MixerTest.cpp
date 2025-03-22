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
	lmms::Mixer* mixer() const { return lmms::Engine::mixer(); }
	lmms::AudioEngine* audioEngine() const { return lmms::Engine::audioEngine(); }

	void addChannels(std::size_t numChannels)
	{
		for (auto i = std::size_t{0}; i < numChannels; ++i)
		{
			mixer()->createChannel();
		}
	}

private slots:
	void initTestCase() { lmms::Engine::init(true); }

	void cleanupTestCase() { lmms::Engine::destroy(); }

	void cleanup() { mixer()->clear(); }

	void testHasMasterChannel() { QCOMPARE(mixer()->containsChannel(0), true); }

	void testCreatedChannelExists()
	{
		const auto index = mixer()->createChannel();

		QCOMPARE(mixer()->containsChannel(index), true);
	}

	void testCreatedChannelHasCorrectDefaults()
	{
		const auto index = mixer()->createChannel();
		const auto channel = mixer()->mixerChannel(index);

		QCOMPARE(channel->m_name, tr("Channel %1").arg(index));
		QCOMPARE(channel->m_volumeModel.value(), 1.f);
		QCOMPARE(channel->m_muteModel.value(), false);
		QCOMPARE(channel->m_soloModel.value(), false);
		QCOMPARE(channel->m_fxChain.empty(), true);
		QCOMPARE(channel->color().has_value(), false);
	}

	void testDeletedChannelDoesNotExist()
	{
		const auto index = mixer()->createChannel();

		mixer()->deleteChannel(index);

		QCOMPARE(mixer()->containsChannel(index), false);
	}

	void testMovingChannelLeftSwapsChannelIndicies()
	{
		addChannels(2);
		const auto channelOne = mixer()->mixerChannel(1);
		const auto channelTwo = mixer()->mixerChannel(2);

		mixer()->moveChannelLeft(2);

		QCOMPARE(channelOne->index(), 2);
		QCOMPARE(channelTwo->index(), 1);
	}

	void testMovingChannelRightSwapsChannelIndicies()
	{
		addChannels(2);
		const auto channelOne = mixer()->mixerChannel(1);
		const auto channelTwo = mixer()->mixerChannel(2);

		mixer()->moveChannelRight(1);

		QCOMPARE(channelOne->index(), 2);
		QCOMPARE(channelTwo->index(), 1);
	}

	void testCreatedRouteHasCorrectSenderReceiver()
	{
		addChannels(2);

		const auto route = mixer()->createChannelSend(1, 2, 1.f);

		QCOMPARE(route->sender()->index(), 1);
		QCOMPARE(route->receiver()->index(), 2);
		QCOMPARE(mixer()->containsSender(1, route), true);
		QCOMPARE(mixer()->containsReceiver(2, route), true);
	}

	void testCreatedRouteHasDefaultAmount()
	{
		addChannels(2);

		const auto route = mixer()->createChannelSend(1, 2, 1.f);

		QCOMPARE(route->amount()->value(), 1.f);
	}

	void testCreatedRouteDoesNotCreateInfiniteLoop()
	{
		addChannels(3);
		mixer()->createChannelSend(1, 2); 
		mixer()->createChannelSend(2, 3);

		const auto infiniteLoopRoute = mixer()->createChannelSend(3, 1);

		QCOMPARE(infiniteLoopRoute, nullptr);
	}

	void testDeletedRouteDoesNotExist()
	{
        addChannels(2);
		const auto route = mixer()->createChannelSend(1, 2, 1.f);

		mixer()->deleteChannelSend(route);

		QCOMPARE(mixer()->containsSender(1, route), false);
		QCOMPARE(mixer()->containsReceiver(2, route), false);
	}

	void testMuteSilencesAudioOutput()
	{
		addChannels(2);
		mixer()->createChannelSend(1, 2);
		mixer()->deleteChannelSend(1, 0);

		const auto channelOne = mixer()->mixerChannel(1);
		std::fill_n(channelOne->m_buffer, audioEngine()->framesPerPeriod(), lmms::SampleFrame{1.f, 1.f});

		channelOne->m_muteModel.setValue(true);
		channelOne->process();

		const auto channelTwo = mixer()->mixerChannel(2);
		const auto muted = std::all_of(channelTwo->m_buffer, channelTwo->m_buffer + audioEngine()->framesPerPeriod(),
			[](const auto& frame) { return frame.left() == 0.f && frame.right() == 0.f; });
		QCOMPARE(muted, true);
	}

	void testUnmuteRestoresAudioOutput()
	{
		addChannels(2);
		mixer()->createChannelSend(1, 2);
		mixer()->deleteChannelSend(1, 0);

		const auto channelOne = mixer()->mixerChannel(1);
		std::fill_n(channelOne->m_buffer, audioEngine()->framesPerPeriod(), lmms::SampleFrame{1.f, 1.f});

		channelOne->m_muteModel.setValue(true);
		channelOne->process();

		channelOne->m_muteModel.setValue(false);
		channelOne->process();

		const auto channelTwo = mixer()->mixerChannel(2);
		const auto muted = std::all_of(channelTwo->m_buffer, channelTwo->m_buffer + audioEngine()->framesPerPeriod(),
			[](const auto& frame) { return frame.left() == 1.f && frame.right() == 1.f; });
		QCOMPARE(muted, false);
	}

	void testSoloMuteOtherChannelsButThoseRouted()
	{
		addChannels(3);
		mixer()->createChannelSend(1, 2);

		mixer()->mixerChannel(1)->m_soloModel.setValue(true);
		mixer()->toggledSolo();

		QCOMPARE(mixer()->mixerChannel(0)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(1)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(2)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(3)->m_muteModel.value(), true);
	}

	void testUnsoloRestoresMuteState()
	{
		addChannels(3);

		mixer()->deleteChannelSend(1, 0);
		mixer()->createChannelSend(1, 2);

		mixer()->mixerChannel(1)->m_soloModel.setValue(true);
		mixer()->toggledSolo();

		mixer()->mixerChannel(1)->m_soloModel.setValue(false);
		mixer()->toggledSolo();

		QCOMPARE(mixer()->mixerChannel(0)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(1)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(2)->m_muteModel.value(), false);
		QCOMPARE(mixer()->mixerChannel(3)->m_muteModel.value(), false);
	}
};

QTEST_GUILESS_MAIN(MixerTest)
#include "MixerTest.moc"
