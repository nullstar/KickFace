#pragma once


#include "../../JuceLibraryCode/JuceHeader.h"
#include <vector>
#include <array>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ShaderProgram.h"



struct Attribute
{
	juce::String m_name;
	GLuint m_numFloats;
	GLuint m_floatOffset;
};






template<class Vertex>
class StaticMesh
{
public:
	StaticMesh(OpenGLContext& openGLContext, const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<Attribute>& attributes);

	void draw(ShaderProgram* pShaderProgram);

private:
	OpenGLContext& m_openGLContext;
	StaticVertexBuffer<Vertex> m_vertexBuffer;
	StaticIndexBuffer m_indexBuffer;
	std::vector<Attribute> m_attributes;
};



template<class Vertex>
StaticMesh<Vertex>::StaticMesh(OpenGLContext& openGLContext, const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::vector<Attribute>& attributes)
	: m_openGLContext(openGLContext)
	, m_vertexBuffer(openGLContext, vertices)
	, m_indexBuffer(openGLContext, indices)
	, m_attributes(attributes)
{
}


template<class Vertex>
void StaticMesh<Vertex>::draw(ShaderProgram* pShaderProgram)
{
	m_vertexBuffer.bind();
	m_indexBuffer.bind();

	for(int i = 0; i < m_attributes.size(); ++i)
	{
		const Attribute& attribute = m_attributes[i];
		int attributeId = pShaderProgram->getAttributeIndex(attribute.m_name);
		m_openGLContext.extensions.glVertexAttribPointer(attributeId, attribute.m_numFloats, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * attribute.m_floatOffset));
		m_openGLContext.extensions.glEnableVertexAttribArray(attributeId);
	}

	glDrawElements(GL_TRIANGLES, m_indexBuffer.getNumIndices(), GL_UNSIGNED_INT, 0);
	
	for(int i = 0; i < m_attributes.size(); ++i)
	{
		const Attribute& attribute = m_attributes[i];
		int attributeId = pShaderProgram->getAttributeIndex(attribute.m_name);
		m_openGLContext.extensions.glDisableVertexAttribArray(attributeId);
	}
}






template<class Vertex>
class DynamicQuadMesh
{
public:
	DynamicQuadMesh(OpenGLContext& openGLContext, GLuint quadCapacity, const std::vector<Attribute>& attributes);

	void setQuad(GLuint quadIndex, const std::array<Vertex, 4>& verts);
	void draw(ShaderProgram* pShaderProgram);
	void draw(ShaderProgram* pShaderProgram, GLuint firstQuadIndex, GLuint lastQuadIndex);

private:
	OpenGLContext& m_openGLContext;
	DynamicVertexBuffer<Vertex> m_vertexBuffer;
	StaticIndexBuffer m_indexBuffer;
	std::vector<Attribute> m_attributes;

	GLuint m_firstDirtyQuad;
	GLuint m_lastDirtyQuad;
};



template<class Vertex>
DynamicQuadMesh<Vertex>::DynamicQuadMesh(OpenGLContext& openGLContext, GLuint quadCapacity, const std::vector<Attribute>& attributes)
	: m_openGLContext(openGLContext)
	, m_vertexBuffer(openGLContext, quadCapacity * 4)
	, m_indexBuffer(openGLContext)
	, m_attributes(attributes)
	, m_firstDirtyQuad(quadCapacity)
	, m_lastDirtyQuad(0)
{
	GLuint vertIndex = 0;
	std::vector<GLuint> indicies;
	indicies.resize(quadCapacity * 6);
	for(GLuint i = 0; i < quadCapacity * 6; i += 6)
	{
		indicies[i + 0] = vertIndex + 0;
		indicies[i + 1] = vertIndex + 1;
		indicies[i + 2] = vertIndex + 2;
		indicies[i + 3] = vertIndex + 0;
		indicies[i + 4] = vertIndex + 2;
		indicies[i + 5] = vertIndex + 3;
		vertIndex += 4;
	}
	m_indexBuffer.initialise(indicies);
}


template<class Vertex>
void DynamicQuadMesh<Vertex>::setQuad(GLuint quadIndex, const std::array<Vertex, 4>& verts)
{
	GLuint vertIndex = quadIndex * 4;
	if(vertIndex < m_vertexBuffer.getVertexCapacity())
	{
		Vertex* pVertBuffer = m_vertexBuffer.getVertexArray();
		pVertBuffer[vertIndex] = verts[0];
		pVertBuffer[vertIndex + 1] = verts[1];
		pVertBuffer[vertIndex + 2] = verts[2];
		pVertBuffer[vertIndex + 3] = verts[3];

		m_firstDirtyQuad = jmin<GLuint>(m_firstDirtyQuad, quadIndex);
		m_lastDirtyQuad = jmax<GLuint>(m_lastDirtyQuad, quadIndex);
	}
}


template<class Vertex>
void DynamicQuadMesh<Vertex>::draw(ShaderProgram* pShaderProgram)
{
	draw(pShaderProgram, 0, (m_vertexBuffer.getVertexCapacity() / 4) - 1);
}


template<class Vertex>
void DynamicQuadMesh<Vertex>::draw(ShaderProgram* pShaderProgram, GLuint firstQuadIndex, GLuint lastQuadIndex)
{
	GLuint quadCapacity = m_indexBuffer.getNumIndices() / 6;
	if(firstQuadIndex >= quadCapacity) { firstQuadIndex = quadCapacity - 1; }
	if(lastQuadIndex >= quadCapacity) { lastQuadIndex = quadCapacity - 1; }

	if(lastQuadIndex >= firstQuadIndex)
	{
		if(m_lastDirtyQuad >= m_firstDirtyQuad)
		{
			m_vertexBuffer.updateVertexArray(m_firstDirtyQuad * 4, (m_lastDirtyQuad + 1) * 4 - 1);
			m_firstDirtyQuad = quadCapacity;
			m_lastDirtyQuad = 0;
		}

		m_vertexBuffer.bind();
		m_indexBuffer.bind();

		for(int i = 0; i < m_attributes.size(); ++i)
		{
			const Attribute& attribute = m_attributes[i];
			int attributeId = pShaderProgram->getAttributeIndex(attribute.m_name);
			m_openGLContext.extensions.glVertexAttribPointer(attributeId, attribute.m_numFloats, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * attribute.m_floatOffset));
			m_openGLContext.extensions.glEnableVertexAttribArray(attributeId);
		}

		glDrawElements(GL_TRIANGLES, (lastQuadIndex - firstQuadIndex + 1) * 6, GL_UNSIGNED_INT, (void*)(firstQuadIndex * 6 * sizeof(GLuint)));

		for(int i = 0; i < m_attributes.size(); ++i)
		{
			const Attribute& attribute = m_attributes[i];
			int attributeId = pShaderProgram->getAttributeIndex(attribute.m_name);
			m_openGLContext.extensions.glDisableVertexAttribArray(attributeId);
		}
	}
}