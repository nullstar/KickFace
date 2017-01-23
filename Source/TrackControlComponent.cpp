#include "TrackControlComponent.h"
#include "PluginProcessor.h"


#define TRACKCONTROL_LISTENMODE_RADIOGROUP 1
#define TRACKCONTROL_DELAY_HEIGHT 3



TrackControlComponent::TrackControlComponent(bool delayOnTop)
	: m_processor(nullptr)
	, m_delayOnTop(delayOnTop)
	, m_listenModeLeftButton("L")
	, m_listenModeSumButton("L+R")
	, m_listenModeRightButton("R")
	, m_invertPhaseButton("Invert Phase")
{
	// initialise delay bar
	m_delayBar.setRange(-SAMPLE_DELAY_RANGE, SAMPLE_DELAY_RANGE, 0.0f);
	m_delayBar.setCurrentValue(0.0f);
	addAndMakeVisible(m_delayBar);

	// initialise listen mode buttons
	m_listenModeLeftButton.setClickingTogglesState(true);
	m_listenModeLeftButton.setRadioGroupId(TRACKCONTROL_LISTENMODE_RADIOGROUP, NotificationType::dontSendNotification);
	m_listenModeLeftButton.setConnectedEdges(Button::ConnectedOnRight);
	m_listenModeLeftButton.addListener(this);
	m_listenModeLeftButton.setTooltip("Show to left channel of this tracks waveform");
	addAndMakeVisible(m_listenModeLeftButton);

	m_listenModeSumButton.setClickingTogglesState(true);
	m_listenModeSumButton.setRadioGroupId(TRACKCONTROL_LISTENMODE_RADIOGROUP, NotificationType::dontSendNotification);
	m_listenModeSumButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	m_listenModeSumButton.addListener(this);
	m_listenModeSumButton.setTooltip("Show the mean of the left and right channels of this tracks waveform");
	addAndMakeVisible(m_listenModeSumButton);

	m_listenModeRightButton.setClickingTogglesState(true);
	m_listenModeRightButton.setRadioGroupId(TRACKCONTROL_LISTENMODE_RADIOGROUP, NotificationType::dontSendNotification);
	m_listenModeRightButton.setConnectedEdges(Button::ConnectedOnLeft);
	m_listenModeRightButton.addListener(this);
	m_listenModeRightButton.setTooltip("Show to right channel of this tracks waveform");
	addAndMakeVisible(m_listenModeRightButton);

	// initialise invert phase button
	m_invertPhaseButton.setClickingTogglesState(true);
	m_invertPhaseButton.setToggleState(false, NotificationType::dontSendNotification);
	m_invertPhaseButton.addListener(this);
	m_invertPhaseButton.setTooltip("Invert the phase of this tracks waveform");
	addAndMakeVisible(m_invertPhaseButton);
}


TrackControlComponent::~TrackControlComponent()
{
	setAudioSource(nullptr);
}


void TrackControlComponent::setAudioSource(KickFaceAudioProcessor* pProcessor)
{
	KickFaceAudioProcessor* pPrevProcessor = m_processor.get();
	if(pPrevProcessor)
	{
		pPrevProcessor->getDelayValue().removeListener(this);
		pPrevProcessor->getInvertPhaseValue().removeListener(this);
		pPrevProcessor->getListenModeValue().removeListener(this);
	}

	m_processor = pProcessor;
	if(pProcessor)
	{
		pProcessor->getDelayValue().addListener(this);
		pProcessor->getInvertPhaseValue().addListener(this);
		pProcessor->getListenModeValue().addListener(this);

		valueChanged(pProcessor->getDelayValue());
		valueChanged(pProcessor->getInvertPhaseValue());
		valueChanged(pProcessor->getListenModeValue());
	}
}


void TrackControlComponent::paint(Graphics& g)
{
}


void TrackControlComponent::resized()
{
	Rectangle<int> bounds = getLocalBounds();

	m_delayBar.setBounds(m_delayOnTop ? bounds.removeFromTop(TRACKCONTROL_DELAY_HEIGHT) : bounds.removeFromBottom(TRACKCONTROL_DELAY_HEIGHT));

	m_listenModeLeftButton.setBounds(bounds.removeFromLeft(20));
	m_listenModeSumButton.setBounds(bounds.removeFromLeft(30));
	m_listenModeRightButton.setBounds(bounds.removeFromLeft(20));
	m_invertPhaseButton.setBounds(bounds);
}


void TrackControlComponent::buttonClicked(Button* pButton)
{
	KickFaceAudioProcessor* pProcessor = m_processor.get();
	if(pProcessor == nullptr)
		return;

	if(pButton == &m_invertPhaseButton)
	{
		pProcessor->getInvertPhaseValue().setValue(m_invertPhaseButton.getToggleState() ? 1.0f : 0.0f);
		return;
	}

	if(pButton == &m_listenModeLeftButton
		|| pButton == &m_listenModeSumButton
		|| pButton == &m_listenModeRightButton)
	{
		E_ListenMode listenMode = E_ListenMode::LeftChannelOnly;
		if(m_listenModeSumButton.getToggleState()) { listenMode = E_ListenMode::SumLeftAndRightChannels; }
		if(m_listenModeRightButton.getToggleState()) { listenMode = E_ListenMode::RightChannelOnly; }
		pProcessor->getListenModeValue().setValue((float)listenMode);
		return;
	}
}


void TrackControlComponent::valueChanged(Value& value)
{
	KickFaceAudioProcessor* pProcessor = m_processor.get();
	if(pProcessor == nullptr)
		return;

	if(value.refersToSameSourceAs(pProcessor->getDelayValue()))
	{
		float delay = (float)value.getValue();
		m_delayBar.setCurrentValue(delay);
		return;
	}

	if(value.refersToSameSourceAs(pProcessor->getInvertPhaseValue()))
	{
		bool invertPhase = (float)value.getValue() > 0.5f;
		m_invertPhaseButton.setToggleState(invertPhase, NotificationType::dontSendNotification);
		return;
	}

	if(value.refersToSameSourceAs(pProcessor->getListenModeValue()))
	{
		E_ListenMode listenMode = E_ListenMode::LeftChannelOnly;
		int listenModeIndex = roundFloatToInt((float)value.getValue());
		if(listenModeIndex >= 0 && listenModeIndex < (int)E_ListenMode::Max)
			listenMode = (E_ListenMode)listenModeIndex;

		switch(listenMode)
		{
		case E_ListenMode::LeftChannelOnly: m_listenModeLeftButton.setToggleState(true, NotificationType::dontSendNotification); break;
		case E_ListenMode::SumLeftAndRightChannels: m_listenModeSumButton.setToggleState(true, NotificationType::dontSendNotification); break;
		case E_ListenMode::RightChannelOnly: m_listenModeRightButton.setToggleState(true, NotificationType::dontSendNotification); break;
		}

		return;
	}
}