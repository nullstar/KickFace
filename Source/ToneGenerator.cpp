#include "ToneGenerator.h"


#define TONEGENERATOR_REQUENCY 80



std::vector<ToneGenerator*> ToneGenerator::m_toneGenerators;


ToneGenerator::ToneGenerator()
	: m_sampleRate(0.0)
	, m_currentAngle(0.0)
{
	if(m_toneGenerators.size() > 0)
		setCurrentAngle(m_toneGenerators[0]->getCurrentAngle());
	m_toneGenerators.push_back(this);
}


ToneGenerator::~ToneGenerator()
{
	for(int i = 0; i < m_toneGenerators.size(); ++i)
	{
		if(m_toneGenerators[i] == this)
		{
			m_toneGenerators[i] = m_toneGenerators[m_toneGenerators.size() - 1];
			m_toneGenerators.pop_back();
			break;
		}
	}
}


void ToneGenerator::prepareToPlay(double sampleRate)
{
	m_sampleRate = sampleRate;
	m_currentAngle = 0.0;
}


float ToneGenerator::generateSample()
{
	// get current sample
	const float currentSample = (float)std::sin(m_currentAngle);

	// update angle
	const double cyclesPerSample = TONEGENERATOR_REQUENCY / m_sampleRate;
	const double angleDelta = cyclesPerSample * 2.0 * juce::double_Pi;

	m_currentAngle += angleDelta;
	if(m_currentAngle > juce::double_Pi)
		m_currentAngle -= juce::double_Pi * 2.0;

	return currentSample;
}


double ToneGenerator::getCurrentAngle() const
{
	return m_currentAngle;
}


void ToneGenerator::setCurrentAngle(double angle)
{
	m_currentAngle = angle;
}