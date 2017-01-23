#pragma once


#include "../JuceLibraryCode/JuceHeader.h"


class SwingBarComponent : public Component
{
public:
	SwingBarComponent();
	~SwingBarComponent();

	void setRange(float minValue, float maxValue, float pivotValue);
	void setCurrentValue(float currentValue);

	float getCurrentValue() const { return m_currentValue; }
	float getMinValue() const { return m_minValue; }
	float getMaxValue() const { return m_maxValue; }
	float getPivotValue() const { return m_pivotValue; }

	enum ColourIds
	{
		backgroundColourId = 0x9000000,    /**< The background colour, behind the bar. */
		foregroundColourId = 0x9000001,    /**< The colour to use to draw the bar itself. LookAndFeel
										   classes will probably use variations on this colour. */
	};

	struct LookAndFeelMethods
	{
		virtual ~LookAndFeelMethods() {}
		virtual void drawSwingBar(Graphics& g, int width, int height, SwingBarComponent& swingBar) = 0;
	};

protected:
	void paint(Graphics&) override;
	void lookAndFeelChanged() override;
	void colourChanged() override;

private:
	float m_currentValue;
	float m_minValue;
	float m_maxValue;
	float m_pivotValue;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwingBarComponent)
};