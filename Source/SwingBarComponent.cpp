#include "SwingBarComponent.h"
#include "KickFaceLookAndFeel.h"


SwingBarComponent::SwingBarComponent()
	: m_currentValue(0.0f)
	, m_minValue(-1.0f)
	, m_maxValue(1.0f)
	, m_pivotValue(0.0f)
{
}


SwingBarComponent::~SwingBarComponent()
{
}


void SwingBarComponent::setRange(float minValue, float maxValue, float pivotValue)
{
	m_minValue = minValue;
	m_maxValue = maxValue;
	m_pivotValue = pivotValue;
	m_currentValue = jlimit(m_minValue, m_maxValue, m_currentValue);
	repaint();
}


void SwingBarComponent::setCurrentValue(float currentValue)
{
	m_currentValue = jlimit(m_minValue, m_maxValue, currentValue);
	repaint();
}


void SwingBarComponent::lookAndFeelChanged()
{
	setOpaque(findColour(backgroundColourId).isOpaque());
}


void SwingBarComponent::colourChanged()
{
	lookAndFeelChanged();
}


void SwingBarComponent::paint(Graphics& g)
{
	KickFaceLookAndFeel* pLookAndFeel = dynamic_cast<KickFaceLookAndFeel*>(&getLookAndFeel());
	if(pLookAndFeel)
		pLookAndFeel->drawSwingBar(g, getWidth(), getHeight(), *this);
}