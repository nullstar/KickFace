#include "KickFaceLookAndFeel.h"
#include "BinaryData.h"
#include "PluginProcessor.h"


KickFaceLookAndFeel::KickFaceLookAndFeel()
	: LookAndFeel_V3()
{
	m_pTypeface = Typeface::createSystemTypefaceFor(BinaryData::XoloniumRegular_ttf, BinaryData::XoloniumRegular_ttfSize).get();
}


Typeface::Ptr KickFaceLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
	return m_pTypeface;
}


void KickFaceLookAndFeel::drawSwingBar(Graphics& g, int width, int height, SwingBarComponent& swingBar)
{
#if USE_LOGGING
	Logger::writeToLog(String("KickFaceLookAndFeel::drawSwingBar"));
#endif

	float pivotRatio = jlimit(0.0f, 1.0f, (swingBar.getPivotValue() - swingBar.getMinValue()) / (swingBar.getMaxValue() - swingBar.getMinValue()));
	float currentRatio = jlimit(0.0f, 1.0f, (swingBar.getCurrentValue() - swingBar.getMinValue()) / (swingBar.getMaxValue() - swingBar.getMinValue()));

	const Colour background(findColour(SwingBarComponent::backgroundColourId));
	const Colour foreground(findColour(SwingBarComponent::foregroundColourId));

	g.fillAll(background);
	g.setColour(foreground);

	int startX = roundFloatToInt(width * pivotRatio);
	int endX = roundFloatToInt(width * currentRatio);
	if(startX > endX) { swapVariables<int>(startX, endX); }

	g.fillRect(startX, 0, endX - startX, height);
}


void KickFaceLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
#if USE_LOGGING
	Logger::writeToLog(String("KickFaceLookAndFeel::drawButtonBackground"));
#endif

	g.setColour(isMouseOverButton ? backgroundColour.brighter(0.05f) : backgroundColour);
	g.fillRect(0, 0, button.getWidth(), button.getHeight());
}