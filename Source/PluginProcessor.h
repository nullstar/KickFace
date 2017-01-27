#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "ToneGenerator.h"


#define USE_PLUGIN_HOST 0
#define USE_TEST_TONE 0
#define USE_LOGGING 1

#define DEFAULT_BPM 100
#define SAMPLE_DELAY_RANGE 2000 
#define PROCESS_INSTANCE_ID_START 2



enum class E_KickFaceError 
{ 
	NoInputChannels = (1 << 0),
	NoPlayheadFound = (1 << 1)
};



enum class E_ListenMode
{
	LeftChannelOnly = 0,
	RightChannelOnly = 1,
	SumLeftAndRightChannels = 2,

	Max
};



class KickFaceAudioProcessor : public AudioProcessor
{
public:
	KickFaceAudioProcessor();
    ~KickFaceAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(AudioSampleBuffer&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

	AudioSampleBuffer* getBeatBuffer();
	int64 getBeatBufferPosition() const;

	Value& getDelayValue() { return m_delayValue; }
	Value& getInvertPhaseValue() { return m_invertPhaseValue; }
	Value& getListenModeValue() { return m_listenModeValue; }

	uint32 getErrorState() const;

	int getInstanceId() const;
	void setGivenName(const String& name);
	const String& getGivenName() const;

private:
	void generateInstanceId();
	void generateGivenName();

	static String invertPhaseToText(float value);
	static float textToInvertPhase(const String& text);
	static String listenModeToText(float value);
	static float textToListenMode(const String& text);

#if USE_LOGGING
	ScopedPointer<juce::FileLogger> m_pFileLogger;
#endif

	int m_instanceId;

	AudioProcessorValueTreeState m_parameters;
	Value m_delayValue;
	Value m_invertPhaseValue;
	Value m_listenModeValue;
	String m_givenName;

	double m_sampleRate;

	AudioSampleBuffer m_beatBuffer;
	int64 m_beatBufferPosition;

	AudioSampleBuffer m_delayBuffer;
	int m_delayBufferPosition;

	int64 m_timeInSamples;

#if USE_TEST_TONE
	ToneGenerator m_testTone;
#endif

	uint32 m_errorState;

	WeakReference<KickFaceAudioProcessor>::Master masterReference;
	friend class WeakReference<KickFaceAudioProcessor>;

	static String s_nameDefs[];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickFaceAudioProcessor)
};
