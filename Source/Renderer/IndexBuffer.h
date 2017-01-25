#pragma once


#include "../../JuceLibraryCode/JuceHeader.h"
#include "ShaderProgram.h"
#include <vector>



class StaticIndexBuffer
{
public:
	StaticIndexBuffer(OpenGLContext& context);
	StaticIndexBuffer(OpenGLContext& context, const std::vector<GLuint>& indices);
	~StaticIndexBuffer();

	void initialise(const std::vector<GLuint>& indices);
	GLuint getNumIndices() const;
	void bind();

private:
	OpenGLContext& m_openGLContext;
	GLuint m_indexBuffer;
	int m_numIndices;
};





class DynamicIndexBuffer
{
public:
	DynamicIndexBuffer(OpenGLContext& context, GLuint indexCapacity);
	~DynamicIndexBuffer();

	GLuint getIndexCapacity() const;
	GLuint* getIndexArray();
	void updateIndexArray(GLuint startIndexIndex, GLuint endIndexIndex);

	void bind();

private:
	OpenGLContext& m_openGLContext;
	GLuint* m_pIndices;
	GLuint m_indexCapacity;
	GLuint m_indexBuffer;
};