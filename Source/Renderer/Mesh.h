#pragma once


#include "../../JuceLibraryCode/JuceHeader.h"
#include <vector>
#include <array>
#include "VertexBuffer.h"


class ShaderProgram;


typedef StaticVertexBuffer<std::array<float, 3>> StaticPositionBuffer;
typedef StaticVertexBuffer<std::array<float, 2>> StaticTexBuffer;
typedef DynamicVertexBuffer<std::array<float, 3>> DynamicPositionBuffer;
typedef DynamicVertexBuffer<std::array<float, 4>> DynamicColourBuffer;


class Mesh
{
public:
	Mesh(OpenGLContext& openGLContext);
	virtual ~Mesh();

	void initialise();
	uint32 getVao() const;

protected:
	OpenGLContext& m_openGLContext;
	uint32 m_vao;
};



class StaticTexQuad : public Mesh
{
public:
	StaticTexQuad(OpenGLContext& openGLContext);
	virtual ~StaticTexQuad();

	void initialise(int positionAttributeIndex, int texAttributeIndex, std::array<float, 2> topLeftPos, std::array<float, 2> dimensions, float depth);
	void render();

private:
	StaticPositionBuffer* m_pPositionBuffer;
	StaticTexBuffer* m_pTexBuffer;
};



class QuadMesh : public Mesh
{
public:
	struct QuadVert
	{
		std::array<float, 3> m_position;
		std::array<float, 4> m_colour;
	};

	QuadMesh(OpenGLContext& openGLContext);
	virtual ~QuadMesh();

	void initialise(int positionAttributeIndex, int colourAttributeIndex, uint16 quadCapacity, uint16 numBuffers);

	bool add(const std::array<QuadVert, 4>& quad);		// verts -> TL, TR, BR, BL
	void render();

	uint16 getRemainingQuadCapacity() const;
	uint16 getNumAddedVertices() const;

private:
	DynamicPositionBuffer* m_pPositionBuffer;
	DynamicColourBuffer* m_pColourBuffer;
	uint16 m_quadCapacity;
};



class FixedQuadMesh : public Mesh
{
public:
	struct QuadVert
	{
		std::array<float, 3> m_position;
		std::array<float, 4> m_colour;
	};

	FixedQuadMesh(OpenGLContext& openGLContext);
	virtual ~FixedQuadMesh();

	void initialise(int positionAttributeIndex, int colourAttributeIndex, uint16 quadCapacity, uint16 numBuffers);

	bool setQuad(const std::array<QuadVert, 4>& quad, int quadIndex);		// verts -> TL, TR, BR, BL
	void render();

	uint16 getNumVertices() const;

private:
	DynamicPositionBuffer* m_pPositionBuffer;
	DynamicColourBuffer* m_pColourBuffer;
};