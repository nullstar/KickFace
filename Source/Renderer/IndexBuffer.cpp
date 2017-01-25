#include "IndexBuffer.h"


StaticIndexBuffer::StaticIndexBuffer(OpenGLContext& context)
	: m_openGLContext(context)
	, m_indexBuffer(0)
	, m_numIndices(0)
{
}


StaticIndexBuffer::StaticIndexBuffer(OpenGLContext& context, const std::vector<GLuint>& indices)
	: m_openGLContext(context)
	, m_indexBuffer(0)
	, m_numIndices(0)
{
	initialise(indices);
}


StaticIndexBuffer::~StaticIndexBuffer()
{
	m_openGLContext.extensions.glDeleteBuffers(1, &m_indexBuffer);
}


void StaticIndexBuffer::initialise(const std::vector<GLuint>& indices)
{
	m_numIndices = indices.size();

	m_openGLContext.extensions.glGenBuffers(1, &m_indexBuffer);
	m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	m_openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		static_cast<GLsizeiptr>(static_cast<size_t>(m_numIndices) * sizeof(GLuint)),
		indices.data(), GL_STATIC_DRAW);
	m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


GLuint StaticIndexBuffer::getNumIndices() const
{
	return m_numIndices;
}


void StaticIndexBuffer::bind()
{
	m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
}





DynamicIndexBuffer::DynamicIndexBuffer(OpenGLContext& context, GLuint indexCapacity)
	: m_openGLContext(context)
	, m_pIndices(nullptr)
	, m_indexCapacity(0)
	, m_indexBuffer(0)
{
	if(m_indexCapacity > 0)
	{
		m_pIndices = new GLuint[m_indexCapacity];

		m_openGLContext.extensions.glGenBuffers(1, &m_indexBuffer);
		m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		m_openGLContext.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			static_cast<GLsizeiptr>(static_cast<size_t>(m_indexCapacity) * sizeof(GLuint)),
			m_pIndices, GL_STATIC_DRAW);
		m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}


DynamicIndexBuffer::~DynamicIndexBuffer()
{
	m_openGLContext.extensions.glDeleteBuffers(1, &m_indexBuffer);

	if(m_pIndices)
		delete[]m_pIndices;
}


GLuint DynamicIndexBuffer::getIndexCapacity() const
{
	return m_indexCapacity;
}


GLuint* DynamicIndexBuffer::getIndexArray()
{
	return m_pIndices;
}


void DynamicIndexBuffer::updateIndexArray(GLuint startIndexIndex, GLuint endIndexIndex)
{
	if(startIndexIndex >= m_indexCapacity) { startIndexIndex = m_indexCapacity - 1; }
	if(endIndexIndex >= m_indexCapacity) { endIndexIndex = m_indexCapacity - 1; }

	if(endIndexIndex > startIndexIndex)
	{
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, m_indexBuffer);
		m_openGLContext.extensions.glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(GLuint) * startIndexIndex),
			static_cast<GLsizeiptr>(sizeof(GLuint) * (endIndexIndex - startIndexIndex)), m_pIndices);
		m_openGLContext.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


void DynamicIndexBuffer::bind()
{
	m_openGLContext.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
}