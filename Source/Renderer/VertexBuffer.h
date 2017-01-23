#pragma once


#include <vector>
#include "../../JuceLibraryCode/JuceHeader.h"
#include "ShaderProgram.h"


#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020


class Mesh;


class VertexBuffer
{
public:
	VertexBuffer(OpenGLContext& openGLContext, int attributeSize, GLenum attributeType);
	~VertexBuffer();

	uint32 getVbo() const { return m_vbo; }

protected:
	OpenGLContext& m_openGLContext;
	uint32 m_vbo;
	int m_attributeSize;
	GLenum m_attributeType;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertexBuffer)
};


template<class T>
class StaticVertexBuffer : public VertexBuffer
{
public:
	StaticVertexBuffer(OpenGLContext& openGLContext, uint32 vao, int attributeIndex, int attributeSize, GLenum attributeType, const std::vector<T>& vertices);
	~StaticVertexBuffer();

	uint32 vertexCount() const { return m_vertexCount; }

protected:
	uint32 m_vertexCount;
	uint32 m_attributeDivisor;
};


template<class T>
class DynamicVertexBuffer : public VertexBuffer
{
public:
	DynamicVertexBuffer(OpenGLContext& openGLContext, uint32 vao, int attributeIndex, int attributeSize, GLenum attributeType, uint32 vertexCapacity);
	~DynamicVertexBuffer();

	void update(bool clearVertexArray);

	uint32 vertexCapacity() const { return m_vertexCapacity; }
	uint32 vertexCount() const { return m_vertexCount; }
	uint32 vertexOffset() const { return m_drawOffset; }
	uint32 numAddedVertices() const { return m_vertices.size(); }

	void clearVertexArray();
	bool addVertex(const T& vertex);
	bool addVertex(const T& vertex, int numVertices);
	bool setVertex(const T& vertex, int vertexIndex);

protected:
	std::vector<T> m_vertices;
	uint32 m_vertexCapacity;
	uint32 m_vertexCount;
	uint32 m_streamOffset;
	uint32 m_drawOffset;
	uint32 m_vao;
	int m_attributeIndex;
	uint32 m_attributeDivisor;
};





template<class T>
StaticVertexBuffer<T>::StaticVertexBuffer(OpenGLContext& openGLContext, uint32 vao, int attributeIndex, int attributeSize, GLenum attributeType, const std::vector<T>& vertices)
	: VertexBuffer(openGLContext, attributeSize, attributeType)
	, m_vertexCount(vertices.size())
{
	// create vbo
	m_openGLContext.extensions.glGenBuffers(1, &m_vbo);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	m_openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(T), (const void*)&vertices[0], GL_STATIC_DRAW);

	// bind mesh attributes
	m_openGLContext.extensions.glBindVertexArray(vao);
	m_openGLContext.extensions.glEnableVertexAttribArray(attributeIndex);
	m_openGLContext.extensions.glVertexAttribPointer(attributeIndex, m_attributeSize, m_attributeType, GL_FALSE, 0, 0);

	// unbind
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_openGLContext.extensions.glBindVertexArray(0);
}


template<class T>
StaticVertexBuffer<T>::~StaticVertexBuffer()
{
	m_vertexCount = 0;
}




template<class T>
DynamicVertexBuffer<T>::DynamicVertexBuffer(OpenGLContext& openGLContext, uint32 vao, int attributeIndex, int attributeSize, GLenum attributeType, uint32 vertexCapacity)
	: VertexBuffer(openGLContext, attributeSize, attributeType)
	, m_vertexCapacity(vertexCapacity)
	, m_streamOffset(0)
	, m_drawOffset(0)
	, m_vao(vao)
	, m_attributeIndex(attributeIndex)
{
	// create vbo
	m_openGLContext.extensions.glGenBuffers(1, &m_vbo);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	m_openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, m_vertexCapacity * sizeof(T), NULL, GL_DYNAMIC_DRAW);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);

	// bind mesh attributes
	m_openGLContext.extensions.glBindVertexArray(m_vao);
	m_openGLContext.extensions.glEnableVertexAttribArray(m_attributeIndex);
	m_openGLContext.extensions.glVertexAttribPointer(m_attributeIndex, m_attributeSize, m_attributeType, GL_FALSE, 0, 0);

	// unbind
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_openGLContext.extensions.glBindVertexArray(0);

	// initialise stream offset to buffer capacity
	m_streamOffset = m_vertexCapacity * sizeof(T);
}


template<class T>
DynamicVertexBuffer<T>::~DynamicVertexBuffer()
{
	m_vertices.clear();
}


template<class T>
void DynamicVertexBuffer<T>::update(bool clearVertexArray)
{
	// get num vertices and bail if nothing to draw
	m_vertexCount = m_vertices.size();
	if(!m_vertexCount)
		return;

	// bind buffer
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	// orphan the buffer if full
	GLuint bufferCapacity = m_vertexCapacity * sizeof(T);
	GLuint streamDataSize = m_vertexCount * sizeof(T);
	if(m_streamOffset + streamDataSize > bufferCapacity)
	{
		// allocate new space
		m_openGLContext.extensions.glBufferData(GL_ARRAY_BUFFER, bufferCapacity, NULL, GL_DYNAMIC_DRAW);

		// reset bound vao
		m_openGLContext.extensions.glBindVertexArray(m_vao);
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		m_openGLContext.extensions.glVertexAttribPointer(m_attributeIndex, m_attributeSize, m_attributeType, GL_FALSE, 0, 0);
		m_openGLContext.extensions.glBindVertexArray(0);

		// reset stream offset
		m_streamOffset = 0;
	}

	// get memory
	void* pVertices = m_openGLContext.extensions.glMapBufferRange(GL_ARRAY_BUFFER, m_streamOffset, streamDataSize, GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	if(!pVertices)
		return;

	// copy vertex data
	memcpy(pVertices, (const void*)&m_vertices[0], streamDataSize);
	if(clearVertexArray)
		m_vertices.clear();

	// unmap buffer
	m_openGLContext.extensions.glUnmapBuffer(GL_ARRAY_BUFFER);
	m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);

	// increment offset
	m_drawOffset = m_streamOffset / sizeof(T);
	m_streamOffset += streamDataSize;
}


template<class T>
void DynamicVertexBuffer<T>::clearVertexArray()
{
	m_vertices.clear();
}


template<class T>
bool DynamicVertexBuffer<T>::addVertex(const T& vertex)
{
	if(m_vertices.size() < m_vertexCapacity)
	{
		m_vertices.push_back(vertex);
		return true;
	}
	return false;
}


template<class T>
bool DynamicVertexBuffer<T>::addVertex(const T& vertex, int numVertices)
{
	if(m_vertices.size() + numVertices <= m_vertexCapacity)
	{
		m_vertices.resize(m_vertices.size() + numVertices, vertex);
		return true;
	}
	return false;
}


template<class T>
bool DynamicVertexBuffer<T>::setVertex(const T& vertex, int vertexIndex)
{
	if(vertexIndex >= 0 && vertexIndex < m_vertices.size())
	{
		m_vertices[vertexIndex] = vertex;
		return true;
	}
	return false;
}