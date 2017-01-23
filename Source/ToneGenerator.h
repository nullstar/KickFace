#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>


class ToneGenerator
{
public:
	ToneGenerator();
	~ToneGenerator();

	void prepareToPlay(double sampleRate);
	float generateSample();

	double getCurrentAngle() const;
	void setCurrentAngle(double angle);

private:
	double m_sampleRate;
	double m_currentAngle;

	static std::vector<ToneGenerator*> m_toneGenerators;
};