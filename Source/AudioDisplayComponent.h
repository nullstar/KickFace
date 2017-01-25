#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Mesh.h"
#include <vector>


class KickFaceAudioProcessor;


class AudioDisplayComponent : public Component, private OpenGLRenderer
{
public:
	AudioDisplayComponent(KickFaceAudioProcessor& processor);
	~AudioDisplayComponent();

	void setRemoteAudioSource(KickFaceAudioProcessor* pProcessor);

private:
	struct TexQuadVert
	{
		float m_position[3];
		float m_texCoord[2];
	};

	struct ColQuadVert
	{
		float m_position[3];
		float m_colour[4];
	};

	struct AudioSourceCache
	{
		int64 m_beatBufferPosition;
		int m_delaySamples;
		bool m_invertPhase;
		int m_listenMode;
	};

	struct AudioSource
	{
		WeakReference<KickFaceAudioProcessor> m_processor;
		ScopedPointer<DynamicQuadMesh<ColQuadVert>> m_pQuadMesh;
		ScopedPointer<StaticMesh<TexQuadVert>> m_pTexQuad;
		Image m_image;
		AudioSourceCache m_prevCache;
	};

	struct CombinedAudioSource
	{
		Array<AudioSource*> m_audioSources;
		ScopedPointer<DynamicQuadMesh<ColQuadVert>> m_pQuadMesh;
		ScopedPointer<StaticMesh<TexQuadVert>> m_pTexQuad;
		Image m_image;
	};

	struct RenderAudioSource
	{
		AudioSource* m_pAudioSource;
		AudioSampleBuffer* m_pBeatBuffer;
		AudioSourceCache m_nextCache;
		float m_sampleSign;
		const float* m_pReadBuffer;
	};

	enum class E_DragMode
	{
		None,
		View,
		Local,
		Remote
	};

	void newOpenGLContextCreated() override;
	void initialiseOpenGL();
	void renderOpenGL() override;
	void renderAudioSource(AudioSource& audioSource, const std::array<float, 4>& colour);
	void renderCombinedAudioSource(CombinedAudioSource& audioSource, const std::array<float, 4>& colour);
	void openGLContextClosing() override;

	void paint(Graphics& g) override;
	void resized() override;

	float sampleBuffer(const float* pReadBuffer, int bufferSize, float samplePosition) const;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;

	OpenGLContext m_openGLContext;
	ScopedPointer<ShaderProgram> m_pQuadMeshShaderProgram;
	ScopedPointer<ShaderProgram> m_pTexQuadShaderProgram;
	ScopedPointer<DynamicQuadMesh<ColQuadVert>> m_pQuadMesh;
	AudioSource m_localAudioSource;
	AudioSource m_remoteAudioSource;
	CombinedAudioSource m_combinedAudioSource;

	float m_viewStartRatio;
	float m_viewEndRatio;
	float m_zoomLevel;
	E_DragMode m_dragMode;
	int m_dragSamples;
	float m_dragViewStart;

	std::vector<RenderAudioSource> m_renderAudioSources;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDisplayComponent)
};