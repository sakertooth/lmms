/*
 * AudioEngine.cpp - device-independent audio engine for LMMS
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioEngine.h"

#include "denormals.h"

#include "lmmsconfig.h"

#include "AudioPort.h"
#include "Mixer.h"
#include "Song.h"
#include "EnvelopeAndLfoParameters.h"
#include "NotePlayHandle.h"
#include "ConfigManager.h"
#include "SamplePlayHandle.h"
#include "MemoryHelper.h"

// platform-specific audio-interface-classes
#include "AudioAlsa.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioSndio.h"
#include "AudioPortAudio.h"
#include "AudioSoundIo.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioDummy.h"

// platform-specific midi-interface-classes
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"
#include "MidiApple.h"
#include "MidiDummy.h"

#include "BufferManager.h"

namespace lmms
{

using LocklessListElement = LocklessList<PlayHandle*>::Element;

static thread_local bool s_renderingThread;
static thread_local bool s_runningChange;

AudioEngine::AudioEngine(bool renderOnly)
	: m_renderOnly(renderOnly)
	, m_framesPerPeriod(AudioNode::DefaultBufferSize)
	, m_inputBufferRead(0)
	, m_inputBufferWrite(1)
	, m_outputBufferRead(nullptr)
	, m_outputBufferWrite(nullptr)
	, m_qualitySettings(qualitySettings::Mode::Draft)
	, m_masterGain(1.0f)
	, m_audioDev(nullptr)
	, m_oldAudioDev(nullptr)
	, m_audioDevStartFailed(false)
	, m_profiler()
	, m_clearSignal(false)
{
	for( int i = 0; i < 2; ++i )
	{
		m_inputBufferFrames[i] = 0;
		m_inputBufferSize[i] = AudioNode::DefaultBufferSize * 100;
		m_inputBuffer[i] = new sampleFrame[AudioNode::DefaultBufferSize * 100];
		BufferManager::clear( m_inputBuffer[i], m_inputBufferSize[i] );
	}

	// now that framesPerPeriod is fixed initialize global BufferManager
	BufferManager::init( m_framesPerPeriod );

	int outputBufferSize = m_framesPerPeriod * sizeof(surroundSampleFrame);
	m_outputBufferRead = static_cast<surroundSampleFrame *>(MemoryHelper::alignedMalloc(outputBufferSize));
	m_outputBufferWrite = static_cast<surroundSampleFrame *>(MemoryHelper::alignedMalloc(outputBufferSize));

	BufferManager::clear(m_outputBufferRead, m_framesPerPeriod);
	BufferManager::clear(m_outputBufferWrite, m_framesPerPeriod);
}




AudioEngine::~AudioEngine()
{
	delete m_midiClient;
	delete m_audioDev;

	MemoryHelper::alignedFree(m_outputBufferRead);
	MemoryHelper::alignedFree(m_outputBufferWrite);

	for (const auto& input : m_inputBuffer)
	{
		delete[] input;
	}
}




void AudioEngine::initDevices()
{
	bool success_ful = false;
	if( m_renderOnly ) {
		m_audioDev = new AudioDummy( success_ful, this );
		m_audioDevName = AudioDummy::name();
		m_midiClient = new MidiDummy;
		m_midiClientName = MidiDummy::name();
	} else {
		m_audioDev = tryAudioDevices();
		m_midiClient = tryMidiClients();
	}
	// Loading audio device may have changed the sample rate
	emit sampleRateChanged();
}




void AudioEngine::startProcessing()
{
	m_audioDev->startProcessing();
}




void AudioEngine::stopProcessing()
{
	m_audioDev->stopProcessing();
}




sample_rate_t AudioEngine::baseSampleRate() const
{
	sample_rate_t sr = ConfigManager::inst()->value( "audioengine", "samplerate" ).toInt();
	if( sr < 44100 )
	{
		sr = 44100;
	}
	return sr;
}




sample_rate_t AudioEngine::outputSampleRate() const
{
	return m_audioDev != nullptr ? m_audioDev->sampleRate() :
							baseSampleRate();
}




sample_rate_t AudioEngine::inputSampleRate() const
{
	return m_audioDev != nullptr ? m_audioDev->sampleRate() :
							baseSampleRate();
}




sample_rate_t AudioEngine::processingSampleRate() const
{
	return outputSampleRate() * m_qualitySettings.sampleRateMultiplier();
}




bool AudioEngine::criticalXRuns() const
{
	return cpuLoad() >= 99 && Engine::getSong()->isExporting() == false;
}

auto AudioEngine::renderNextBuffer() -> AudioNode::Buffer
{
	const auto lock = std::lock_guard{m_changeMutex};

	m_profiler.startPeriod();
	s_renderingThread = true;

	Engine::getSong()->processNextBuffer();

	const auto masterChannel = Engine::mixer()->mixerChannel(0);
	auto task = masterChannel->pull(m_audioProcessor, m_framesPerPeriod);

	const auto output = task.get();
	emit nextAudioBuffer(output.data());

	EnvelopeAndLfoParameters::instances()->trigger();
	Controller::triggerFrameCounter();
	AutomatableModel::incrementPeriodCounter();

	s_renderingThread = false;
	m_profiler.finishPeriod(processingSampleRate(), m_framesPerPeriod);

	return output;
}




void AudioEngine::swapBuffers()
{
	m_inputBufferWrite = (m_inputBufferWrite + 1) % 2;
	m_inputBufferRead = (m_inputBufferRead + 1) % 2;
	m_inputBufferFrames[m_inputBufferWrite] = 0;

	std::swap(m_outputBufferRead, m_outputBufferWrite);
	BufferManager::clear(m_outputBufferWrite, m_framesPerPeriod);
}


void AudioEngine::clear()
{
	m_clearSignal = true;
}




void AudioEngine::clearNewPlayHandles()
{
	// TODO: Remove function when new system is used
}


AudioEngine::StereoSample AudioEngine::getPeakValues(sampleFrame * ab, const f_cnt_t frames) const
{
	sample_t peakLeft = 0.0f;
	sample_t peakRight = 0.0f;

	for (f_cnt_t f = 0; f < frames; ++f)
	{
		float const absLeft = std::abs(ab[f][0]);
		float const absRight = std::abs(ab[f][1]);
		if (absLeft > peakLeft)
		{
			peakLeft = absLeft;
		}

		if (absRight > peakRight)
		{
			peakRight = absRight;
		}
	}

	return StereoSample(peakLeft, peakRight);
}




void AudioEngine::changeQuality(const struct qualitySettings & qs)
{
	// don't delete the audio-device
	stopProcessing();

	m_qualitySettings = qs;
	m_audioDev->applyQualitySettings();

	emit sampleRateChanged();
	emit qualitySettingsChanged();

	startProcessing();
}




void AudioEngine::doSetAudioDevice( AudioDevice * _dev )
{
	// TODO: Use shared_ptr here in the future.
	// Currently, this is safe, because this is only called by
	// ProjectRenderer, and after ProjectRenderer calls this function,
	// it does not access the old device anymore.
	if( m_audioDev != m_oldAudioDev ) {delete m_audioDev;}

	if( _dev )
	{
		m_audioDev = _dev;
	}
	else
	{
		printf( "param _dev == NULL in AudioEngine::setAudioDevice(...). "
					"Trying any working audio-device\n" );
		m_audioDev = tryAudioDevices();
	}
}

void AudioEngine::setAudioDevice(AudioDevice* _dev, const struct qualitySettings& _qs, bool startNow)
{
	stopProcessing();

	m_qualitySettings = _qs;

	doSetAudioDevice( _dev );

	emit qualitySettingsChanged();
	emit sampleRateChanged();

	if (startNow) { startProcessing(); }
}




void AudioEngine::storeAudioDevice()
{
	if( !m_oldAudioDev )
	{
		m_oldAudioDev = m_audioDev;
	}
}




void AudioEngine::restoreAudioDevice()
{
	if( m_oldAudioDev && m_audioDev != m_oldAudioDev )
	{
		stopProcessing();
		delete m_audioDev;

		m_audioDev = m_oldAudioDev;
		emit sampleRateChanged();

		startProcessing();
	}
	m_oldAudioDev = nullptr;
}




void AudioEngine::removeAudioPort(AudioPort * port)
{
	// TODO: Remove function when new system is used
}


bool AudioEngine::addPlayHandle( PlayHandle* handle )
{
	// TODO: Remove function when new system is used
	return false;
}


void AudioEngine::removePlayHandle(PlayHandle * ph)
{
	// TODO: Remove function when new system is used
}




void AudioEngine::removePlayHandlesOfTypes(Track * track, PlayHandle::Types types)
{
	// TODO: Remove function when new system is used
}




void AudioEngine::requestChangeInModel()
{
	if (s_renderingThread || s_runningChange) { return; }
	m_changeMutex.lock();
	s_runningChange = true;
}

void AudioEngine::doneChangeInModel()
{
	if (s_renderingThread || !s_runningChange) { return; }
	m_changeMutex.unlock();
	s_runningChange = false;
}

bool AudioEngine::isAudioDevNameValid(QString name)
{
#ifdef LMMS_HAVE_SDL
	if (name == AudioSdl::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_ALSA
	if (name == AudioAlsa::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if (name == AudioPulseAudio::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if (name == AudioOss::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if (name == AudioSndio::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if (name == AudioJack::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if (name == AudioPortAudio::name())
	{
		return true;
	}
#endif


#ifdef LMMS_HAVE_SOUNDIO
	if (name == AudioSoundIo::name())
	{
		return true;
	}
#endif

	if (name == AudioDummy::name())
	{
		return true;
	}

	return false;
}

bool AudioEngine::isMidiDevNameValid(QString name)
{
#ifdef LMMS_HAVE_ALSA
	if (name == MidiAlsaSeq::name() || name == MidiAlsaRaw::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if (name == MidiJack::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_OSS
	if (name == MidiOss::name())
	{
		return true;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if (name == MidiSndio::name())
	{
		return true;
	}
#endif

#ifdef LMMS_BUILD_WIN32
	if (name == MidiWinMM::name())
	{
		return true;
	}
#endif

#ifdef LMMS_BUILD_APPLE
    if (name == MidiApple::name())
    {
		return true;
    }
#endif

    if (name == MidiDummy::name())
    {
		return true;
    }

	return false;
}

AudioDevice * AudioEngine::tryAudioDevices()
{
	bool success_ful = false;
	AudioDevice * dev = nullptr;
	QString dev_name = ConfigManager::inst()->value( "audioengine", "audiodev" );
	if( !isAudioDevNameValid( dev_name ) )
	{
		dev_name = "";
	}

	m_audioDevStartFailed = false;

#ifdef LMMS_HAVE_SDL
	if( dev_name == AudioSdl::name() || dev_name == "" )
	{
		dev = new AudioSdl( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSdl::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_ALSA
	if( dev_name == AudioAlsa::name() || dev_name == "" )
	{
		dev = new AudioAlsa( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioAlsa::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PULSEAUDIO
	if( dev_name == AudioPulseAudio::name() || dev_name == "" )
	{
		dev = new AudioPulseAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPulseAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_OSS
	if( dev_name == AudioOss::name() || dev_name == "" )
	{
		dev = new AudioOss( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioOss::name();
			return dev;
		}
		delete dev;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( dev_name == AudioSndio::name() || dev_name == "" )
	{
		dev = new AudioSndio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSndio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_JACK
	if( dev_name == AudioJack::name() || dev_name == "" )
	{
		dev = new AudioJack( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioJack::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_PORTAUDIO
	if( dev_name == AudioPortAudio::name() || dev_name == "" )
	{
		dev = new AudioPortAudio( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioPortAudio::name();
			return dev;
		}
		delete dev;
	}
#endif


#ifdef LMMS_HAVE_SOUNDIO
	if( dev_name == AudioSoundIo::name() || dev_name == "" )
	{
		dev = new AudioSoundIo( success_ful, this );
		if( success_ful )
		{
			m_audioDevName = AudioSoundIo::name();
			return dev;
		}
		delete dev;
	}
#endif


	// add more device-classes here...
	//dev = new audioXXXX( SAMPLE_RATES[m_qualityLevel], success_ful, this );
	//if( sucess_ful )
	//{
	//	return dev;
	//}
	//delete dev

	if( dev_name != AudioDummy::name() )
	{
		printf( "No audio-driver working - falling back to dummy-audio-"
			"driver\nYou can render your songs and listen to the output "
			"files...\n" );

		m_audioDevStartFailed = true;
	}

	m_audioDevName = AudioDummy::name();

	return new AudioDummy( success_ful, this );
}




MidiClient * AudioEngine::tryMidiClients()
{
	QString client_name = ConfigManager::inst()->value( "audioengine", "mididev" );
	if( !isMidiDevNameValid( client_name ) )
	{
		client_name = "";
	}

#ifdef LMMS_HAVE_ALSA
	if( client_name == MidiAlsaSeq::name() || client_name == "" )
	{
		auto malsas = new MidiAlsaSeq;
		if( malsas->isRunning() )
		{
			m_midiClientName = MidiAlsaSeq::name();
			return malsas;
		}
		delete malsas;
	}

	if( client_name == MidiAlsaRaw::name() || client_name == "" )
	{
		auto malsar = new MidiAlsaRaw;
		if( malsar->isRunning() )
		{
			m_midiClientName = MidiAlsaRaw::name();
			return malsar;
		}
		delete malsar;
	}
#endif

#ifdef LMMS_HAVE_JACK
	if( client_name == MidiJack::name() || client_name == "" )
	{
		auto mjack = new MidiJack;
		if( mjack->isRunning() )
		{
			m_midiClientName = MidiJack::name();
			return mjack;
		}
		delete mjack;
	}
#endif

#ifdef LMMS_HAVE_OSS
	if( client_name == MidiOss::name() || client_name == "" )
	{
		auto moss = new MidiOss;
		if( moss->isRunning() )
		{
			m_midiClientName = MidiOss::name();
			return moss;
		}
		delete moss;
	}
#endif

#ifdef LMMS_HAVE_SNDIO
	if( client_name == MidiSndio::name() || client_name == "" )
	{
		MidiSndio * msndio = new MidiSndio;
		if( msndio->isRunning() )
		{
			m_midiClientName = MidiSndio::name();
			return msndio;
		}
		delete msndio;
	}
#endif

#ifdef LMMS_BUILD_WIN32
	if( client_name == MidiWinMM::name() || client_name == "" )
	{
		MidiWinMM * mwmm = new MidiWinMM;
//		if( moss->isRunning() )
		{
			m_midiClientName = MidiWinMM::name();
			return mwmm;
		}
		delete mwmm;
	}
#endif

#ifdef LMMS_BUILD_APPLE
    printf( "trying midi apple...\n" );
    if( client_name == MidiApple::name() || client_name == "" )
    {
        MidiApple * mapple = new MidiApple;
        m_midiClientName = MidiApple::name();
        printf( "Returning midi apple\n" );
        return mapple;
    }
    printf( "midi apple didn't work: client_name=%s\n", client_name.toUtf8().constData());
#endif

	if(client_name != MidiDummy::name())
	{
		if (client_name.isEmpty())
		{
			printf("Unknown MIDI-client. ");
		}
		else
		{
			printf("Couldn't create %s MIDI-client. ", client_name.toUtf8().constData());
		}
		printf("Will use dummy-MIDI-client.\n");
	}

	m_midiClientName = MidiDummy::name();

	return new MidiDummy;
}

} // namespace lmms
