#pragma once


#include "../../JuceLibraryCode/JuceHeader.h"


typedef juce::HashMap<int, int> VariableMap;


class ShaderProgram
{
public:
	ShaderProgram(OpenGLContext& openGLContext);
	virtual ~ShaderProgram();

	bool load(const char* pVertexSource, const char* pFragmentSource);

	int getAttributeIndex(const juce::String& name);
	int getUniformIndex(const juce::String& name);
	void useProgram() const;

private:
	OpenGLContext& m_openGLContext;
	uint32 m_programId;
	VariableMap m_variableMap;

	uint32 createShader(const char* pShaderSource, GLenum shaderType) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShaderProgram)
};

