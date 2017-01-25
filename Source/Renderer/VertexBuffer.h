#pragma once


#include "../../JuceLibraryCode/JuceHeader.h"
#include <vector>



template<class Vertex>
class StaticVertexBuffer
{
public:
	StaticVertexBuffer(OpenGLContext& context, const std::vector<Vertex>& vertices);
	~StaticVertexBuffer();

	void bind();

private:
	OpenGLContext& m_openGLContext;
	GLuint m_vertexBuffer;
};


template<class Vertex>
StaticVertexBuffer<Vertex>::StaticVertexBuffer(OpenGLContext& context, const std::vector<Vertex>& vertices)
	: m_openGLContext(context)
	, m_vertexBuffer(0)
{
	m_openGLContext.extensions.glGenBuffers(1, &m_vertexBuffer);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	m_openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(static_cast<size_t>(vertices.size()) * sizeof(Vertex)),
		vertices.data(), GL_STATIC_DRAW);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
}


template<class Vertex>
StaticVertexBuffer<Vertex>::~StaticVertexBuffer()
{
	m_openGLContext.extensions.glDeleteBuffers(1, &m_vertexBuffer);
}


template<class Vertex>
void StaticVertexBuffer<Vertex>::bind()
{
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
}




template<class Vertex>
class DynamicVertexBuffer
{
public:
	DynamicVertexBuffer(OpenGLContext& context, GLuint vertexCapacity);
	~DynamicVertexBuffer();

	GLuint getVertexCapacity() const;
	Vertex* getVertexArray();
	void updateVertexArray(GLuint startVertexIndex, GLuint lastVertexIndex);

	void bind();

private:
	OpenGLContext& m_openGLContext;
	Vertex* m_pVertices;
	GLuint m_vertexCapacity;
	GLuint m_vertexBuffer;
};


template<class Vertex>
DynamicVertexBuffer<Vertex>::DynamicVertexBuffer(OpenGLContext& context, GLuint vertexCapacity)
	: m_openGLContext(context)
	, m_pVertices(nullptr)
	, m_vertexCapacity(vertexCapacity)
	, m_vertexBuffer(0)
{
	if(m_vertexCapacity > 0)
	{
		m_pVertices = new Vertex[m_vertexCapacity];

		m_openGLContext.extensions.glGenBuffers(1, &m_vertexBuffer);
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		m_openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER,
			static_cast<GLsizeiptr>(static_cast<size_t>(m_vertexCapacity) * sizeof(Vertex)),
			m_pVertices, GL_DYNAMIC_DRAW);
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


template<class Vertex>
DynamicVertexBuffer<Vertex>::~DynamicVertexBuffer()
{
	m_openGLContext.extensions.glDeleteBuffers(1, &m_vertexBuffer);

	if(m_pVertices)
		delete []m_pVertices;
}


template<class Vertex>
GLuint DynamicVertexBuffer<Vertex>::getVertexCapacity() const
{
	return m_vertexCapacity;
}


template<class Vertex>
Vertex* DynamicVertexBuffer<Vertex>::getVertexArray()
{
	return m_pVertices;
}


template<class Vertex>
void DynamicVertexBuffer<Vertex>::updateVertexArray(GLuint startVertexIndex, GLuint lastVertexIndex)
{
	if(startVertexIndex >= m_vertexCapacity) { startVertexIndex = m_vertexCapacity - 1; }
	if(lastVertexIndex >= m_vertexCapacity) { lastVertexIndex = m_vertexCapacity - 1; }

	if(lastVertexIndex >= startVertexIndex)
	{
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		m_openGLContext.extensions.glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * startVertexIndex),
			static_cast<GLsizeiptr>(sizeof(Vertex) * (lastVertexIndex - startVertexIndex + 1)), m_pVertices);
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


template<class Vertex>
void DynamicVertexBuffer<Vertex>::bind()
{
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
}