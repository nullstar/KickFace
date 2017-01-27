#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GlobalProcessorArray.h"
#include "Math.h"
#include <vector>



#if USE_LOGGING
String g_logWelcomeMessage =
"--------------------------------------------------------------------\n"
"-----------------------------START----------------------------------\n"
"KICKFACE v1.0";
#endif



String KickFaceAudioProcessor::s_nameDefs[] = {
	"Athelstan",
	"Quentin",
	"Jenson",
	"Winston",
	"Gilbert",
	"Bernard",
	"Theodore",
	"Cedric",
	"Dorian",
	"Edmund",
	"Hector",
	"Neville",
	"Percival",
	"Rueben",
	"Sheldon",
	"Vincent" };



KickFaceAudioProcessor::KickFaceAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
	, m_instanceId(-1)
	, m_parameters(*this, nullptr)
	, m_sampleRate(0.0)
	, m_beatBufferPosition(0)
	, m_delayBufferPosition(0)
	, m_errorState(0)
{
#if USE_LOGGING
	m_pFileLogger = FileLogger::createDateStampedLogger("KickFace", "KickFace_", ".txt", g_logWelcomeMessage);
#endif

	setLatencySamples(SAMPLE_DELAY_RANGE);

	m_parameters.createAndAddParameter("delay", "Delay", "delay", NormalisableRange<float>(-SAMPLE_DELAY_RANGE, SAMPLE_DELAY_RANGE, 1.0f), 0.0f, nullptr, nullptr);
	m_parameters.createAndAddParameter("invertPhase", "InvertPhase", "invertPhase", NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f, invertPhaseToText, textToInvertPhase);
	m_parameters.createAndAddParameter("listenMode", "ListenMode", "listenMode", NormalisableRange<float>(0.0f, (float)E_ListenMode::Max, 1.0f), (float)E_ListenMode::LeftChannelOnly, listenModeToText, textToListenMode);
	m_parameters.state = ValueTree(Identifier("KickFaceValueTree"));

	m_delayValue = m_parameters.getParameterAsValue("delay");
	m_invertPhaseValue = m_parameters.getParameterAsValue("invertPhase");
	m_listenModeValue = m_parameters.getParameterAsValue("listenMode");

	generateInstanceId();
	generateGivenName();

	GlobalProcessorArray::addProcessor(this);
}


KickFaceAudioProcessor::~KickFaceAudioProcessor()
{
	GlobalProcessorArray::removeProcessor(this);
	masterReference.clear();

#if USE_LOGGING
	Logger::setCurrentLogger(nullptr);
	m_pFileLogger = nullptr;
#endif
}


const String KickFaceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}


bool KickFaceAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}


bool KickFaceAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}


double KickFaceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}


int KickFaceAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}


int KickFaceAudioProcessor::getCurrentProgram()
{
    return 0;
}


void KickFaceAudioProcessor::setCurrentProgram(int index)
{
}


const String KickFaceAudioProcessor::getProgramName(int index)
{
    return String();
}


void KickFaceAudioProcessor::changeProgramName(int index, const String& newName)
{
}


void KickFaceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// initialise state
	m_sampleRate = sampleRate;
	m_errorState = 0;

	// prepare beat buffer
	m_beatBuffer.clear();

	// prepare delay buffer
	m_delayBuffer.setSize(2, 2 * SAMPLE_DELAY_RANGE);
	m_delayBuffer.clear();
	m_delayBufferPosition = 0;

	// reset time
	m_timeInSamples = 0;

#if USE_TEST_TONE
	// prepare test tone
	m_testTone.prepareToPlay(sampleRate);
#endif

#if USE_LOGGING
	Logger::writeToLog(String("prepareToPlay : sampleRate ") + String(m_sampleRate));
#endif
}


void KickFaceAudioProcessor::releaseResources()
{
	m_delayBuffer.setSize(0, 0);
	m_beatBuffer.setSize(0, 0);

#if USE_LOGGING
	Logger::writeToLog(String("releaseResources"));
#endif
}


#ifndef JucePlugin_PreferredChannelConfigurations
bool KickFaceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if(layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if(layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif


void KickFaceAudioProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // clear excess output channels
    for(int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

	// check we have input channels
	if(totalNumInputChannels <= 0)
	{
		m_errorState |= (uint32)E_KickFaceError::NoInputChannels;
		return;
	}

	// get channel data
	float* pChannelData[2];
	pChannelData[0] = (totalNumInputChannels > 0) ? buffer.getWritePointer(0) : nullptr;
	pChannelData[1] = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

	// update test tone
#if USE_TEST_TONE
	double testToneAngle = m_testTone.getCurrentAngle();
	for(int channel = 0; channel < jmin(jmin(totalNumInputChannels, 2), totalNumOutputChannels); ++channel)
	{
		// write test tone to channel data
		m_testTone.setCurrentAngle(testToneAngle);
		for(int i = 0; i < buffer.getNumSamples(); ++i)
			pChannelData[channel][i] = m_testTone.generateSample() * 0.5f;
	}
#endif

	// update beat buffer
	AudioPlayHead::CurrentPositionInfo posInfo;
	if(getPlayHead()->getCurrentPosition(posInfo))
	{
#if USE_PLUGIN_HOST
		double bpm = DEFAULT_BPM;
		m_timeInSamples = m_timeInSamples;
#else
		double bpm = posInfo.bpm;
		m_timeInSamples = posInfo.timeInSamples;
#endif

		const int64 numSamplesPerBeat = (bpm > 0.0) ? (int64)ceil(m_sampleRate * 60.0 / bpm) : 0;
		m_beatBuffer.setSize(1, numSamplesPerBeat, true, true, true);

		if(m_beatBuffer.getNumSamples() > 0 && numSamplesPerBeat > 0)
		{
			E_ListenMode listenMode = (E_ListenMode)juce::roundFloatToInt((float)m_listenModeValue.getValue());
			if(totalNumInputChannels > 1 && listenMode == E_ListenMode::SumLeftAndRightChannels)
			{
				int64 numSamplesWritten = 0;
				while(numSamplesWritten < buffer.getNumSamples())
				{
					// write data into beat buffer
					const int64 numSamplesFromBeatStart = (m_timeInSamples + numSamplesWritten) % numSamplesPerBeat;
					const int64 numSamplesToWrite = jmin<int64>(buffer.getNumSamples(), numSamplesPerBeat - numSamplesFromBeatStart);
					float* pWriteData = m_beatBuffer.getWritePointer(0, numSamplesFromBeatStart);
					FloatVectorOperations::copyWithMultiply(pWriteData, pChannelData[0] + numSamplesWritten, 0.5f, numSamplesToWrite);
					FloatVectorOperations::addWithMultiply(pWriteData, pChannelData[1] + numSamplesWritten, 0.5f, numSamplesToWrite);
					numSamplesWritten += numSamplesToWrite;
				}
			}
			else
			{
				const float* pReadData = (totalNumInputChannels == 1) ? pChannelData[0] : pChannelData[(int)listenMode];

				int64 numSamplesWritten = 0;
				while(numSamplesWritten < buffer.getNumSamples())
				{
					// write data into beat buffer
					const int64 numSamplesFromBeatStart = (m_timeInSamples + numSamplesWritten) % numSamplesPerBeat;
					const int64 numSamplesToWrite = jmin<int64>(buffer.getNumSamples(), numSamplesPerBeat - numSamplesFromBeatStart);
					float* pWriteData = m_beatBuffer.getWritePointer(0, numSamplesFromBeatStart);
					FloatVectorOperations::copy(pWriteData, pReadData + numSamplesWritten, numSamplesToWrite);
					numSamplesWritten += numSamplesToWrite;
				}
			}
		}
#if USE_LOGGING
		else
		{
			Logger::writeToLog(String("processBlock -> empty beat buffer : beatBufferSize ") + String(m_beatBuffer.getNumSamples())
				+ String(" : sampleRate ") + String(m_sampleRate)
				+ String(" : bpm ") + String(bpm)
				+ String(" : numSamplesPerBeat ") + String(numSamplesPerBeat));
		}
#endif

		m_beatBufferPosition = (m_timeInSamples + buffer.getNumSamples()) % numSamplesPerBeat;

#if USE_PLUGIN_HOST
		m_timeInSamples += buffer.getNumSamples();
#endif
	}
	else
	{
		m_errorState |= (uint32)E_KickFaceError::NoPlayheadFound;
		m_beatBufferPosition = 0;
	}

    // update and output delay line
	float delay = (float)m_delayValue.getValue();
	bool invertPhase = (float)m_invertPhaseValue.getValue() > 0.5f ? true : false;
    for(int channel = 0; channel < jmin(jmin(totalNumInputChannels, 2), totalNumOutputChannels); ++channel)
    {
		// write to delay buffer
		int numSamplesWritten = 0;
		while(numSamplesWritten < buffer.getNumSamples())
		{
			int delayWritePosition = (m_delayBufferPosition + numSamplesWritten) % m_delayBuffer.getNumSamples();
			float* pDelayData = m_delayBuffer.getWritePointer(channel, delayWritePosition);
			int numSamplesToWrite = juce::jmin(buffer.getNumSamples() - numSamplesWritten, m_delayBuffer.getNumSamples() - delayWritePosition);
			FloatVectorOperations::copy(pDelayData, pChannelData[channel] + numSamplesWritten, numSamplesToWrite);
			numSamplesWritten += numSamplesToWrite;
		}

		// write to output buffer
		if(m_delayBuffer.getNumSamples() > 0)
		{
			numSamplesWritten = 0;
			while(numSamplesWritten < buffer.getNumSamples())
			{
				int delayReadPosition = Math::positiveModulo(m_delayBufferPosition + numSamplesWritten - delay - SAMPLE_DELAY_RANGE, m_delayBuffer.getNumSamples());
				const float* pDelayData = m_delayBuffer.getReadPointer(channel, delayReadPosition);
				int numSamplesToWrite = juce::jmin(buffer.getNumSamples() - numSamplesWritten, m_delayBuffer.getNumSamples() - delayReadPosition);

				if(invertPhase)
					FloatVectorOperations::copyWithMultiply(pChannelData[channel] + numSamplesWritten, pDelayData, -1.0f, numSamplesToWrite);
				else
					FloatVectorOperations::copy(pChannelData[channel] + numSamplesWritten, pDelayData, numSamplesToWrite);

				numSamplesWritten += numSamplesToWrite;
			}
		}
#if USE_LOGGING
		else
		{
			Logger::writeToLog(String("processBlock -> empty delay buffer : delayBufferSize ") + String(m_delayBuffer.getNumSamples()));
		}
#endif
    }

	// update delay buffer position
	m_delayBufferPosition = (m_delayBufferPosition + buffer.getNumSamples()) % m_delayBuffer.getNumSamples();
}


bool KickFaceAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}


AudioProcessorEditor* KickFaceAudioProcessor::createEditor()
{
    return new KickFaceAudioProcessorEditor(*this);
}


void KickFaceAudioProcessor::getStateInformation(MemoryBlock& destData)
{
	ScopedPointer<juce::XmlElement> pXml = new juce::XmlElement("KickFaceState");
	pXml->setAttribute("GivenName", m_givenName);
	pXml->addChildElement(m_parameters.state.createXml());
	copyXmlToBinary(*pXml, destData);
}


void KickFaceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	ScopedPointer<juce::XmlElement> pXml(getXmlFromBinary(data, sizeInBytes));
	if(pXml != nullptr)
	{
		if(pXml->hasTagName("KickFaceState"))
		{
			if(pXml->hasAttribute("GivenName"))
				m_givenName = pXml->getStringAttribute("GivenName");

			for(int childIndex = 0; childIndex < pXml->getNumChildElements(); ++childIndex)
			{
				juce::XmlElement* pChildElement = pXml->getChildElement(childIndex);
				if(pChildElement->hasTagName(m_parameters.state.getType()))
					m_parameters.state = ValueTree::fromXml(*pChildElement);
			}
		}
	}

	m_delayValue.referTo(m_parameters.getParameterAsValue("delay"));
	m_invertPhaseValue.referTo(m_parameters.getParameterAsValue("invertPhase"));
	m_listenModeValue.referTo(m_parameters.getParameterAsValue("listenMode"));
}


AudioSampleBuffer* KickFaceAudioProcessor::getBeatBuffer()
{
	return &m_beatBuffer;
}


int64 KickFaceAudioProcessor::getBeatBufferPosition() const
{
	return m_beatBufferPosition;
}


uint32 KickFaceAudioProcessor::getErrorState() const
{
	return m_errorState;
}


int KickFaceAudioProcessor::getInstanceId() const
{
	return m_instanceId;
}


void KickFaceAudioProcessor::setGivenName(const String& name)
{
	m_givenName = name;
	GlobalProcessorArray::processorGivenNameChanged(this);
}


const String& KickFaceAudioProcessor::getGivenName() const
{
	return m_givenName;
}


void KickFaceAudioProcessor::generateInstanceId()
{
	m_instanceId = -1;

	int instanceId = -1;
	static juce::Random rand;
	while(true)
	{
		instanceId = rand.nextInt(Range<int>(PROCESS_INSTANCE_ID_START, 0x7FFFFFFF));
		KickFaceAudioProcessor* pFoundProcessor = GlobalProcessorArray::getProcessorById(instanceId);
		if(pFoundProcessor == nullptr)
			break;
	}

	m_instanceId = instanceId;
}


void KickFaceAudioProcessor::generateGivenName()
{
	static juce::Random rand;
	static std::vector<int> nameDefIndices;
	nameDefIndices.clear();

	const std::vector<WeakReference<KickFaceAudioProcessor>>& processors = GlobalProcessorArray::getProcessors();

	int numNameDefs = sizeof(s_nameDefs) / sizeof(String);
	for(int nameIndex = 0; nameIndex < numNameDefs; ++nameIndex)
	{
		bool usedName = false;
		for(int processIndex = 0; processIndex < processors.size(); ++processIndex)
		{
			if(processors[processIndex]->m_givenName == s_nameDefs[nameIndex])
				usedName = true;
		}

		if(!usedName)
			nameDefIndices.push_back(nameIndex);
	}

	if(nameDefIndices.size() > 0)
	{
		m_givenName = s_nameDefs[rand.nextInt(nameDefIndices.size())];
		return;
	}

	m_givenName.clear();
	for(int i = 0; i < 10; ++i)
		m_givenName += (char)((int)('a') + rand.nextInt(26));
}


String KickFaceAudioProcessor::invertPhaseToText(float value)
{
	return (value < 0.5) ? "Normal" : "Inverted";
}


float KickFaceAudioProcessor::textToInvertPhase(const String& text)
{
	if(text == "Normal") { return 0.0f; }
	if(text == "Inverted") { return 1.0f; }
	return 0.0f;
}


String KickFaceAudioProcessor::listenModeToText(float value)
{
	int valueInt = juce::roundFloatToInt(value);
	switch(valueInt)
	{
	case (int)E_ListenMode::LeftChannelOnly: return "Left";
	case (int)E_ListenMode::RightChannelOnly: return "Right";
	case (int)E_ListenMode::SumLeftAndRightChannels: return "Sum";
	}
	return "Left";
}


float KickFaceAudioProcessor::textToListenMode(const String& text)
{
	if(text == "Left") { return (float)E_ListenMode::LeftChannelOnly; }
	if(text == "Right") { return (float)E_ListenMode::RightChannelOnly; }
	if(text == "Sum") { return (float)E_ListenMode::SumLeftAndRightChannels; }
	return 0.0f;
}


// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickFaceAudioProcessor();
}
