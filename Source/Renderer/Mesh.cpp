#include "Mesh.h"
#include <vector>


Mesh::Mesh(OpenGLContext& openGLContext)
	: m_openGLContext(openGLContext)
	, m_vao(0)
{
}


Mesh::~Mesh()
{
	m_openGLContext.extensions.glDeleteVertexArrays(1, &m_vao);
}


void Mesh::initialise()
{
	m_openGLContext.extensions.glGenVertexArrays(1, &m_vao);
}


uint32 Mesh::getVao() const
{
	return m_vao;
}




StaticTexQuad::StaticTexQuad(OpenGLContext& openGLContext)
	: Mesh(openGLContext)
	, m_pPositionBuffer(nullptr)
	, m_pTexBuffer(nullptr)
{
}


StaticTexQuad::~StaticTexQuad()
{
	delete m_pPositionBuffer;
	delete m_pTexBuffer;
}


void StaticTexQuad::initialise(int positionAttributeIndex, int texAttributeIndex, std::array<float, 2> topLeftPos, std::array<float, 2> dimensions, float depth)
{
	Mesh::initialise();

	std::vector<std::array<float, 3>> positions;
	positions.resize(6);
	positions[0][0] = topLeftPos[0];
	positions[0][1] = topLeftPos[1];
	positions[0][2] = depth;
	positions[1][0] = topLeftPos[0] + dimensions[0];
	positions[1][1] = topLeftPos[1];
	positions[1][2] = depth;
	positions[2][0] = topLeftPos[0] + dimensions[0];
	positions[2][1] = topLeftPos[1] + dimensions[1];
	positions[2][2] = depth;
	positions[3][0] = topLeftPos[0];
	positions[3][1] = topLeftPos[1];
	positions[3][2] = depth;
	positions[4][0] = topLeftPos[0] + dimensions[0];
	positions[4][1] = topLeftPos[1] + dimensions[1];
	positions[4][2] = depth;
	positions[5][0] = topLeftPos[0];
	positions[5][1] = topLeftPos[1] + dimensions[1];
	positions[5][2] = depth;
	m_pPositionBuffer = new StaticPositionBuffer(m_openGLContext, getVao(), positionAttributeIndex, 3, GL_FLOAT, positions);

	std::vector<std::array<float, 2>> texCoords;
	texCoords.resize(6);
	texCoords[0][0] = 0.0f;
	texCoords[0][1] = 0.0f;
	texCoords[1][0] = 1.0f;
	texCoords[1][1] = 0.0f;
	texCoords[2][0] = 1.0f;
	texCoords[2][1] = 1.0f;
	texCoords[3][0] = 0.0f;
	texCoords[3][1] = 0.0f;
	texCoords[4][0] = 1.0f;
	texCoords[4][1] = 1.0f;
	texCoords[5][0] = 0.0f;
	texCoords[5][1] = 1.0f;
	m_pTexBuffer = new StaticTexBuffer(m_openGLContext, getVao(), texAttributeIndex, 2, GL_FLOAT, texCoords);
}


void StaticTexQuad::render()
{
	m_openGLContext.extensions.glBindVertexArray(getVao());

	// draw 
	glDrawArrays(GL_TRIANGLES, 0, m_pPositionBuffer->vertexCount());

	// unbind
	m_openGLContext.extensions.glBindVertexArray(0);
}




QuadMesh::QuadMesh(OpenGLContext& openGLContext)
	: Mesh(openGLContext)
	, m_pPositionBuffer(nullptr)
	, m_pColourBuffer(nullptr)
	, m_quadCapacity(0)
{
}


QuadMesh::~QuadMesh()
{
	delete m_pPositionBuffer;
	delete m_pColourBuffer;
}


void QuadMesh::initialise(int positionAttributeIndex, int colourAttributeIndex, uint16 quadCapacity, uint16 numBuffers)
{
	Mesh::initialise();
	m_pPositionBuffer = new DynamicPositionBuffer(m_openGLContext, getVao(), positionAttributeIndex, 3, GL_FLOAT, numBuffers * quadCapacity * 6);
	m_pColourBuffer = new DynamicColourBuffer(m_openGLContext, getVao(), colourAttributeIndex, 4, GL_FLOAT, numBuffers * quadCapacity * 6);
	m_quadCapacity = quadCapacity;
}


bool QuadMesh::add(const std::array<QuadVert, 4>& quad)
{
	// check buffer isn't full
	if(m_pPositionBuffer->numAddedVertices() + 6 > m_quadCapacity * 6)
		return false;

	// add to position buffer
	m_pPositionBuffer->addVertex(quad[0].m_position);
	m_pPositionBuffer->addVertex(quad[1].m_position);
	m_pPositionBuffer->addVertex(quad[2].m_position);

	m_pPositionBuffer->addVertex(quad[0].m_position);
	m_pPositionBuffer->addVertex(quad[2].m_position);
	m_pPositionBuffer->addVertex(quad[3].m_position);

	// add to colour buffer
	m_pColourBuffer->addVertex(quad[0].m_colour);
	m_pColourBuffer->addVertex(quad[1].m_colour);
	m_pColourBuffer->addVertex(quad[2].m_colour);

	m_pColourBuffer->addVertex(quad[0].m_colour);
	m_pColourBuffer->addVertex(quad[2].m_colour);
	m_pColourBuffer->addVertex(quad[3].m_colour);

	return true;
}


void QuadMesh::render()
{
	m_pPositionBuffer->update(true);
	m_pColourBuffer->update(true);

	m_openGLContext.extensions.glBindVertexArray(getVao());

	// draw 
	glDrawArrays(GL_TRIANGLES, m_pPositionBuffer->vertexOffset(), m_pPositionBuffer->vertexCount());

	// unbind
	m_openGLContext.extensions.glBindVertexArray(0);
}


uint16 QuadMesh::getRemainingQuadCapacity() const
{
	return m_quadCapacity - (m_pPositionBuffer->numAddedVertices() / 6);
}


uint16 QuadMesh::getNumAddedVertices() const
{
	return m_pPositionBuffer->numAddedVertices();
}





FixedQuadMesh::FixedQuadMesh(OpenGLContext& openGLContext)
	: Mesh(openGLContext)
	, m_pPositionBuffer(nullptr)
	, m_pColourBuffer(nullptr)
{
}


FixedQuadMesh::~FixedQuadMesh()
{
	delete m_pPositionBuffer;
	delete m_pColourBuffer;
}


void FixedQuadMesh::initialise(int positionAttributeIndex, int colourAttributeIndex, uint16 quadCapacity, uint16 numBuffers)
{
	Mesh::initialise();
	m_pPositionBuffer = new DynamicPositionBuffer(m_openGLContext, getVao(), positionAttributeIndex, 3, GL_FLOAT, numBuffers * quadCapacity * 4);
	m_pColourBuffer = new DynamicColourBuffer(m_openGLContext, getVao(), colourAttributeIndex, 4, GL_FLOAT, numBuffers * quadCapacity * 4);

	std::array<float, 3> defaultVert;
	defaultVert[0] = 0.0f;
	defaultVert[1] = 0.0f;
	defaultVert[2] = 0.0f;
	m_pPositionBuffer->addVertex(defaultVert, quadCapacity * 4);

	std::array<float, 4> defaultColour;
	defaultColour[0] = 0.0f;
	defaultColour[1] = 0.0f;
	defaultColour[2] = 0.0f;
	defaultColour[3] = 0.0f;
	m_pColourBuffer->addVertex(defaultColour, quadCapacity * 4);
}


bool FixedQuadMesh::setQuad(const std::array<QuadVert, 4>& quad, int quadIndex)
{
	int vertIndex = quadIndex * 4;

	// set position buffer
	m_pPositionBuffer->setVertex(quad[0].m_position, vertIndex);
	m_pPositionBuffer->setVertex(quad[1].m_position, vertIndex + 1);
	m_pPositionBuffer->setVertex(quad[2].m_position, vertIndex + 2);
	m_pPositionBuffer->setVertex(quad[3].m_position, vertIndex + 3);

	// set colour buffer
	m_pColourBuffer->setVertex(quad[0].m_colour, vertIndex);
	m_pColourBuffer->setVertex(quad[1].m_colour, vertIndex + 1);
	m_pColourBuffer->setVertex(quad[2].m_colour, vertIndex + 2);
	m_pColourBuffer->setVertex(quad[3].m_colour, vertIndex + 3);

	return true;
}


void FixedQuadMesh::render()
{
	m_pPositionBuffer->update(false);
	m_pColourBuffer->update(false);

	m_openGLContext.extensions.glBindVertexArray(getVao());

	// draw 
	glDrawArrays(GL_QUADS, m_pPositionBuffer->vertexOffset(), m_pPositionBuffer->vertexCount());

	// unbind
	m_openGLContext.extensions.glBindVertexArray(0);
}


uint16 FixedQuadMesh::getNumVertices() const
{
	return m_pPositionBuffer->numAddedVertices();
}


