#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "AudioDisplayComponent.h"
#include "TrackControlComponent.h"
#include "GlobalProcessorArray.h"


class KickFaceAudioProcessorEditor : public AudioProcessorEditor, public juce::TextEditor::Listener, public juce::Button::Listener, 
	public juce::ComboBox::Listener, public GlobalProcessorArray::Listener
{
public:
	KickFaceAudioProcessorEditor(KickFaceAudioProcessor&);
	~KickFaceAudioProcessorEditor();

    void paint(Graphics&) override;
    void resized() override;

private:
    KickFaceAudioProcessor& m_processor;

	juce::Image m_logoImage;
	juce::Image m_infoImage;
	juce::URL m_infoUrl;

	ImageButton m_infoButton;
	TextEditor m_nameEditor;
	ComboBox m_remoteSourceListBox;
	ScopedPointer<AudioDisplayComponent> m_pAudioDisplay;
	TrackControlComponent m_trackControl;
	TrackControlComponent m_remoteTrackControl;

	ScopedPointer<juce::LookAndFeel> m_pLocalLookAndFeel;
	ScopedPointer<juce::LookAndFeel> m_pRemoteLookAndFeel;

	void refreshProcessorList();

	virtual void textEditorTextChanged(TextEditor& textEditor) override;
	virtual void buttonClicked(Button* pButton) override;
	virtual void comboBoxChanged(ComboBox* pComboBox) override;

	virtual void processorAdded(KickFaceAudioProcessor* pProcessor) override;
	virtual void processorRemoved(KickFaceAudioProcessor* pProcessor) override;
	virtual void processorGivenNameChanged(KickFaceAudioProcessor* pProcessor) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KickFaceAudioProcessorEditor)
};
