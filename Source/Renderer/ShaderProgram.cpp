#include "ShaderProgram.h"


ShaderProgram::ShaderProgram(OpenGLContext& openGLContext)
	: m_openGLContext(openGLContext)
	, m_programId(0)
{
}


ShaderProgram::~ShaderProgram()
{
	m_openGLContext.extensions.glDeleteProgram(m_programId);
	m_variableMap.clear();
}


bool ShaderProgram::load(const char* pVertexSource, const char* pFragmentSource)
{
	// create shaders
	GLuint vertexShaderId = createShader(pVertexSource, GL_VERTEX_SHADER);
	GLuint fragmentShaderId = createShader(pFragmentSource, GL_FRAGMENT_SHADER);

	// create shader program
	GLuint shaderProgramId = m_openGLContext.extensions.glCreateProgram();

	// link shader program
	m_openGLContext.extensions.glAttachShader(shaderProgramId, vertexShaderId);
	m_openGLContext.extensions.glAttachShader(shaderProgramId, fragmentShaderId);
	m_openGLContext.extensions.glLinkProgram(shaderProgramId);

	// check program
	int result;
	m_openGLContext.extensions.glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &result);
	if(result == GL_FALSE)
	{
		char shaderErrorMessage[2048];
		m_openGLContext.extensions.glGetProgramInfoLog(shaderProgramId, 2048, NULL, shaderErrorMessage);

		juce::String errorMessage = "Failed to link shader program : ";
		errorMessage += shaderErrorMessage;
		Logger::getCurrentLogger()->writeToLog(errorMessage);

		m_openGLContext.extensions.glDeleteShader(shaderProgramId);
	}
	else
	{
		// initialise shader program
		m_programId = shaderProgramId;
	}

	// release un-needed resources
	m_openGLContext.extensions.glDeleteShader(vertexShaderId);
	m_openGLContext.extensions.glDeleteShader(fragmentShaderId);

	// return shader program name
	return result == GL_TRUE;
}


int ShaderProgram::getAttributeIndex(const juce::String& name)
{
	const int nameHash = name.hashCode();
	if(m_variableMap.contains(nameHash))
		return m_variableMap[nameHash];

	int attributeLocation = m_openGLContext.extensions.glGetAttribLocation(m_programId, name.getCharPointer());
	m_variableMap.set(nameHash, attributeLocation);
	return attributeLocation;
}


int ShaderProgram::getUniformIndex(const juce::String& name)
{
	const int nameHash = name.hashCode();
	if(m_variableMap.contains(nameHash))
		return m_variableMap[nameHash];
	
	int uniformLocation = m_openGLContext.extensions.glGetUniformLocation(m_programId, name.getCharPointer());
	m_variableMap.set(nameHash, uniformLocation);
	return uniformLocation;
}


void ShaderProgram::useProgram() const
{
	m_openGLContext.extensions.glUseProgram(m_programId);
}


uint32 ShaderProgram::createShader(const char* pShaderSource, GLenum shaderType) const
{
	// create the shader
	uint32 shaderId = m_openGLContext.extensions.glCreateShader(shaderType);

	// compile Shader
	m_openGLContext.extensions.glShaderSource(shaderId, 1, &pShaderSource, NULL);
	m_openGLContext.extensions.glCompileShader(shaderId);

	// check shader
	int result;
	m_openGLContext.extensions.glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE)
	{
		char shaderErrorMessage[2048];
		m_openGLContext.extensions.glGetShaderInfoLog(shaderId, 2048, NULL, shaderErrorMessage);

		juce::String errorMessage = "Failed to compile shader : ";
		errorMessage += shaderErrorMessage;
		Logger::getCurrentLogger()->writeToLog(errorMessage);

		m_openGLContext.extensions.glDeleteShader(shaderId);
		return 0;
	}

	return shaderId;
}