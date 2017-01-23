#include "VertexBuffer.h"



VertexBuffer::VertexBuffer(OpenGLContext& openGLContext, int attributeSize, GLenum attributeType)
	: m_openGLContext(openGLContext)
	, m_vbo(0)
	, m_attributeSize(attributeSize)
	, m_attributeType(attributeType)
{
}


VertexBuffer::~VertexBuffer()
{
	if(m_vbo)
	{
		m_openGLContext.extensions.glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
}


