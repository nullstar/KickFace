#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "SwingBarComponent.h"


class KickFaceLookAndFeel : public LookAndFeel_V3
	, public SwingBarComponent::LookAndFeelMethods
{
public:
	KickFaceLookAndFeel();
	virtual ~KickFaceLookAndFeel() {}

	virtual Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

	virtual void drawSwingBar(Graphics& g, int width, int height, SwingBarComponent& swingBar);
	virtual void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown);

private:
	Typeface::Ptr m_pTypeface;
};