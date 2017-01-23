#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "SwingBarComponent.h"


class KickFaceAudioProcessor;


class TrackControlComponent : public Component, public juce::Button::Listener, public juce::Value::Listener
{
public:
	TrackControlComponent(bool delayOnTop);
	~TrackControlComponent();

	void setAudioSource(KickFaceAudioProcessor* pProcessor);

private:
	void paint(Graphics& g) override;
	void resized() override;

	virtual void buttonClicked(Button* pButton) override;
	virtual void valueChanged(Value& value) override;

	WeakReference<KickFaceAudioProcessor> m_processor;

	bool m_delayOnTop;
	SwingBarComponent m_delayBar;
	TextButton m_listenModeLeftButton;
	TextButton m_listenModeSumButton;
	TextButton m_listenModeRightButton;
	TextButton m_invertPhaseButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlComponent)
};