#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "KickFaceLookAndFeel.h"
#include "BinaryData.h"


#define LISTEN_MODE_RADIO_GROUP 1



KickFaceAudioProcessorEditor::KickFaceAudioProcessorEditor(KickFaceAudioProcessor& p)
    : AudioProcessorEditor(&p)
	, m_processor(p)
	, m_infoUrl("https://nullstar.github.io/")
	, m_remoteSourceListBox("Remote Source")
	, m_trackControl(false)
	, m_remoteTrackControl(true)
	, m_pLocalLookAndFeel(nullptr)
	, m_pRemoteLookAndFeel(nullptr)
{
#if USE_LOGGING
	Logger::writeToLog(String("KickFaceAudioProcessorEditor::construct"));
#endif

	GlobalProcessorArray::addListener(this);

	// initialise images
	m_logoImage = ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);
	m_infoImage = ImageCache::getFromMemory(BinaryData::info_png, BinaryData::info_pngSize);

	// create look and feel
	juce::Colour localBaseColour(141, 21, 74);
	juce::Colour localHilightColour(248, 114, 9);
	juce::Colour remoteBaseColour(40, 80, 90);
	juce::Colour remoteHilightColour(176, 189, 19);

	m_pLocalLookAndFeel = new KickFaceLookAndFeel();
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::backgroundColourId, localBaseColour);
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::highlightColourId, localHilightColour);
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::highlightedTextColourId, juce::Colour::greyLevel(0.1f));
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::textColourId, juce::Colour::greyLevel(0.1f));
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::focusedOutlineColourId, localBaseColour.darker(0.5f));
	m_pLocalLookAndFeel->setColour(TextEditor::ColourIds::outlineColourId, localBaseColour.darker(0.5f));
	m_pLocalLookAndFeel->setColour(SwingBarComponent::ColourIds::backgroundColourId, localBaseColour);
	m_pLocalLookAndFeel->setColour(SwingBarComponent::ColourIds::foregroundColourId, localHilightColour);
	m_pLocalLookAndFeel->setColour(TextButton::ColourIds::buttonColourId, localBaseColour);
	m_pLocalLookAndFeel->setColour(TextButton::ColourIds::buttonOnColourId, localHilightColour);
	KickFaceLookAndFeel::setDefaultLookAndFeel(m_pLocalLookAndFeel);

	m_pRemoteLookAndFeel = new KickFaceLookAndFeel();
	m_pRemoteLookAndFeel->setColour(ComboBox::ColourIds::backgroundColourId, remoteBaseColour);
	m_pRemoteLookAndFeel->setColour(ComboBox::ColourIds::outlineColourId, remoteBaseColour.darker(0.5f));
	m_pRemoteLookAndFeel->setColour(ComboBox::ColourIds::arrowColourId, remoteHilightColour);
	m_pRemoteLookAndFeel->setColour(ComboBox::ColourIds::buttonColourId, remoteHilightColour);
	m_pRemoteLookAndFeel->setColour(ComboBox::ColourIds::textColourId, Colour::greyLevel(0.1f));
	m_pRemoteLookAndFeel->setColour(SwingBarComponent::ColourIds::backgroundColourId, remoteBaseColour);
	m_pRemoteLookAndFeel->setColour(SwingBarComponent::ColourIds::foregroundColourId, remoteHilightColour);
	m_pRemoteLookAndFeel->setColour(TextButton::ColourIds::buttonColourId, remoteBaseColour);
	m_pRemoteLookAndFeel->setColour(TextButton::ColourIds::buttonOnColourId, remoteHilightColour);

	// initialise info button
	m_infoButton.setImages(true, false, true,
		m_infoImage, 1.0f, juce::Colour(80, 80, 90),
		m_infoImage, 1.0f, juce::Colour(147, 147, 159),
		m_infoImage, 1.0f, localHilightColour);
	m_infoButton.addListener(this);
	addAndMakeVisible(m_infoButton);

	// initialise name editor
	m_nameEditor.setLookAndFeel(m_pLocalLookAndFeel);
	m_nameEditor.addListener(this);
	m_nameEditor.setMultiLine(false, false);
	m_nameEditor.setReadOnly(false);
	m_nameEditor.setText(m_processor.getGivenName(), false);
	m_nameEditor.setTooltip("The name of this instance of KickFace");
	addAndMakeVisible(m_nameEditor);

	// initialise remote source list box
	refreshProcessorList();
	m_remoteSourceListBox.setLookAndFeel(m_pRemoteLookAndFeel);
	m_remoteSourceListBox.addListener(this); 
	m_remoteSourceListBox.setTooltip("Select a remote instance of KickFace by it's name to compare its waveform to this tracks");
	addAndMakeVisible(m_remoteSourceListBox);

	// initialise audio display
	m_pAudioDisplay = new AudioDisplayComponent(p);
	addAndMakeVisible(m_pAudioDisplay);

	// initialise track control
	m_trackControl.setLookAndFeel(m_pLocalLookAndFeel);
	m_trackControl.setAudioSource(&m_processor);
	addAndMakeVisible(m_trackControl);

	m_remoteTrackControl.setLookAndFeel(m_pRemoteLookAndFeel);
	addAndMakeVisible(m_remoteTrackControl);

	// set size and colour
	setSize(420, 300);
}


KickFaceAudioProcessorEditor::~KickFaceAudioProcessorEditor()
{
#if USE_LOGGING
	Logger::writeToLog(String("KickFaceAudioProcessorEditor::destruct"));
#endif

	GlobalProcessorArray::removeListener(this);
	m_pAudioDisplay = nullptr;
	m_pLocalLookAndFeel = nullptr;
	m_pRemoteLookAndFeel = nullptr;
}


void KickFaceAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(Colour(43, 50, 50));

	Rectangle<int> bounds = getLocalBounds();
	m_infoButton.setTopRightPosition(bounds.getWidth() - 15, 15);

	bounds.reduce(9, 9);
	bounds.removeFromTop(50);
	g.setColour(Colour::greyLevel(0.1f));
	g.fillRect(bounds);


	g.drawImageAt(m_logoImage, 2, 1);
}


void KickFaceAudioProcessorEditor::resized()
{
	Rectangle<int> bounds = getLocalBounds();
	bounds.reduce(10, 10);
	bounds.removeFromTop(50);

	m_nameEditor.setBounds(bounds.removeFromTop(20));
	m_trackControl.setBounds(bounds.removeFromTop(20));

	m_remoteSourceListBox.setBounds(bounds.removeFromBottom(20));
	m_remoteTrackControl.setBounds(bounds.removeFromBottom(20));

	m_pAudioDisplay->setBounds(bounds);
}


void KickFaceAudioProcessorEditor::refreshProcessorList()
{
	int selectedId = m_remoteSourceListBox.getSelectedId();
	m_remoteSourceListBox.clear(NotificationType::dontSendNotification);
	m_remoteSourceListBox.addItem("NO REMOTE SOURCE", 1);
	
	const std::vector<WeakReference<KickFaceAudioProcessor>>& processors = GlobalProcessorArray::getProcessors();
	for(int processorIndex = 0; processorIndex < processors.size(); ++processorIndex)
	{
		const KickFaceAudioProcessor* pProcessor = processors[processorIndex].get();
		if(pProcessor != &m_processor)
			m_remoteSourceListBox.addItem(pProcessor->getGivenName().length() ? pProcessor->getGivenName() : ".", pProcessor->getInstanceId());
	}

	if(m_remoteSourceListBox.indexOfItemId(selectedId) == -1)
		m_remoteSourceListBox.setSelectedId(1, NotificationType::sendNotification);
	else
		m_remoteSourceListBox.setSelectedId(selectedId, NotificationType::dontSendNotification);
}


void KickFaceAudioProcessorEditor::textEditorTextChanged(TextEditor& textEditor)
{
	if(&textEditor == &m_nameEditor)
	{
		m_processor.setGivenName(m_nameEditor.getText());
		return;
	}
}


void KickFaceAudioProcessorEditor::buttonClicked(Button* pButton)
{
	if(pButton == &m_infoButton)
	{
		if(m_infoUrl.isWellFormed())
			m_infoUrl.launchInDefaultBrowser();
	}
}


void KickFaceAudioProcessorEditor::comboBoxChanged(ComboBox* pComboBox)
{
	if(pComboBox == &m_remoteSourceListBox)
	{
		int selectedId = m_remoteSourceListBox.getSelectedId();
		KickFaceAudioProcessor* pProcessor = GlobalProcessorArray::getProcessorById(selectedId);
		m_pAudioDisplay->setRemoteAudioSource(pProcessor);
		m_remoteTrackControl.setAudioSource(pProcessor);
	}
}


void KickFaceAudioProcessorEditor::processorAdded(KickFaceAudioProcessor* pProcessor)
{
	refreshProcessorList();
}


void KickFaceAudioProcessorEditor::processorRemoved(KickFaceAudioProcessor* pProcessor)
{
	refreshProcessorList();
}


void KickFaceAudioProcessorEditor::processorGivenNameChanged(KickFaceAudioProcessor* pProcessor)
{
	refreshProcessorList();
}
