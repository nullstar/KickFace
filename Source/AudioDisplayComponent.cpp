#include "AudioDisplayComponent.h"
#include "PluginProcessor.h"
#include "Math.h"


#define AUDIODISPLAY_NEAR 0.0f
#define AUDIODISPLAY_FAR 1.0f
#define AUDIODISPLAY_NUM_QUADS 500
#define AUDIODISPLAY_UPSCALE 1



const char* gQuadMeshVertexShaderSource = 
"attribute vec3 v_position;\
attribute vec4 v_colour;\
\
varying vec4 f_colour;\
\
void main()\
{\
	gl_Position = vec4(v_position, 1.0f);\
	f_colour = v_colour;\
}";


const char* gQuadMeshFragmentShaderSource = 
"varying vec4 f_colour;\
\
void main()\
{\
	gl_FragColor = f_colour;\
}";



const char* gTexQuadVertexShaderSource =
"attribute vec3 v_position;\
attribute vec2 v_texCoord;\
\
varying vec2 f_texCoord;\
\
void main()\
{\
	gl_Position = vec4(v_position, 1.0f);\
	f_texCoord = v_texCoord;\
}";


const char* gTexQuadFragmentShaderSource =
"varying vec2 f_texCoord;\
\
uniform sampler2D texture0;\
\
void main()\
{\
	gl_FragColor = texture2D(texture0, f_texCoord);\
}";




AudioDisplayComponent::AudioDisplayComponent(KickFaceAudioProcessor& processor)
	: m_viewStartRatio(0.0f)
	, m_viewEndRatio(1.0f)
	, m_zoomLevel(0.0f)
	, m_dragMode(E_DragMode::None)
	, m_dragSamples(0)
	, m_resizeImages(false)
{
	m_localAudioSource.m_processor = &processor;
	m_localAudioSource.m_pQuadMesh = nullptr;
	m_localAudioSource.m_prevCache.m_beatBufferPosition = 0;
	m_localAudioSource.m_prevCache.m_delaySamples = 0;
	m_localAudioSource.m_prevCache.m_invertPhase = false;
	m_localAudioSource.m_prevCache.m_listenMode = (int)E_ListenMode::LeftChannelOnly;

	m_remoteAudioSource.m_processor = nullptr;
	m_remoteAudioSource.m_pQuadMesh = nullptr;
	m_remoteAudioSource.m_prevCache.m_beatBufferPosition = 0;
	m_remoteAudioSource.m_prevCache.m_delaySamples = 0;
	m_remoteAudioSource.m_prevCache.m_invertPhase = false;
	m_remoteAudioSource.m_prevCache.m_listenMode = (int)E_ListenMode::LeftChannelOnly;

	m_combinedAudioSource.m_audioSources.add(&m_localAudioSource);
	m_combinedAudioSource.m_audioSources.add(&m_remoteAudioSource);
	m_combinedAudioSource.m_pQuadMesh = nullptr;

	m_openGLContext.setComponentPaintingEnabled(false);
	m_openGLContext.setContinuousRepainting(true);
	m_openGLContext.setRenderer(this);
	m_openGLContext.attachTo(*this);
}


AudioDisplayComponent::~AudioDisplayComponent()
{
	m_openGLContext.detach();
	m_openGLContext.setRenderer(nullptr);
}


void AudioDisplayComponent::setRemoteAudioSource(KickFaceAudioProcessor* pProcessor)
{
	m_remoteAudioSource.m_processor = pProcessor;
	if(pProcessor)
	{
		m_remoteAudioSource.m_prevCache.m_beatBufferPosition = pProcessor->getBeatBufferPosition();
		m_remoteAudioSource.m_prevCache.m_delaySamples = (float)pProcessor->getDelayValue().getValue();
		m_remoteAudioSource.m_prevCache.m_invertPhase = (float)pProcessor->getInvertPhaseValue().getValue() > 0.5f;
		m_remoteAudioSource.m_prevCache.m_listenMode = roundFloatToInt((float)pProcessor->getListenModeValue().getValue());
	}
	else
	{
		m_remoteAudioSource.m_prevCache.m_beatBufferPosition = 0;
		m_remoteAudioSource.m_prevCache.m_delaySamples = 0;
		m_remoteAudioSource.m_prevCache.m_invertPhase = false;
		m_remoteAudioSource.m_prevCache.m_listenMode = 0;
	}
}


void AudioDisplayComponent::newOpenGLContextCreated()
{
	m_pQuadMeshShaderProgram = nullptr;
	m_pTexQuadShaderProgram = nullptr;
	m_pQuadMesh = nullptr;
	m_localAudioSource.m_pQuadMesh = nullptr;
	m_remoteAudioSource.m_pQuadMesh = nullptr;
	m_combinedAudioSource.m_pQuadMesh = nullptr;
	m_localAudioSource.m_pTexQuad = nullptr;
	m_remoteAudioSource.m_pTexQuad = nullptr;
	m_combinedAudioSource.m_pTexQuad = nullptr;
}


void AudioDisplayComponent::initialiseOpenGL()
{
	if(m_pQuadMeshShaderProgram != nullptr)
		return;

	m_pQuadMeshShaderProgram = new ShaderProgram(m_openGLContext);
	if(m_pQuadMeshShaderProgram->load(gQuadMeshVertexShaderSource, gQuadMeshFragmentShaderSource))
	{
		std::vector<Attribute> colQuadAttributes;
		colQuadAttributes.resize(2);

		colQuadAttributes[0].m_name = "v_position";
		colQuadAttributes[0].m_numFloats = 3;
		colQuadAttributes[0].m_floatOffset = 0;

		colQuadAttributes[1].m_name = "v_colour";
		colQuadAttributes[1].m_numFloats = 4;
		colQuadAttributes[1].m_floatOffset = 3;

		m_pQuadMesh = new DynamicQuadMesh<ColQuadVert>(m_openGLContext, 3, colQuadAttributes);
		m_localAudioSource.m_pQuadMesh = new DynamicQuadMesh<ColQuadVert>(m_openGLContext, AUDIODISPLAY_NUM_QUADS, colQuadAttributes);
		m_remoteAudioSource.m_pQuadMesh = new DynamicQuadMesh<ColQuadVert>(m_openGLContext, AUDIODISPLAY_NUM_QUADS, colQuadAttributes);
		m_combinedAudioSource.m_pQuadMesh = new DynamicQuadMesh<ColQuadVert>(m_openGLContext, AUDIODISPLAY_NUM_QUADS, colQuadAttributes);
	}

	m_pTexQuadShaderProgram = new ShaderProgram(m_openGLContext);
	if(m_pTexQuadShaderProgram->load(gTexQuadVertexShaderSource, gTexQuadFragmentShaderSource))
	{
		std::vector<TexQuadVert> texQuadVerts;
		texQuadVerts.resize(4);

		texQuadVerts[0].m_position[0] = -1.0f;
		texQuadVerts[0].m_position[1] = -1.0f;
		texQuadVerts[0].m_position[2] = 0.5f;
		texQuadVerts[0].m_texCoord[0] = 0.0f;
		texQuadVerts[0].m_texCoord[1] = 0.0f;

		texQuadVerts[1].m_position[0] = 1.0f;
		texQuadVerts[1].m_position[1] = -1.0f;
		texQuadVerts[1].m_position[2] = 0.5f;
		texQuadVerts[1].m_texCoord[0] = 1.0f;
		texQuadVerts[1].m_texCoord[1] = 0.0f;

		texQuadVerts[2].m_position[0] = 1.0f;
		texQuadVerts[2].m_position[1] = 1.0f;
		texQuadVerts[2].m_position[2] = 0.5f;
		texQuadVerts[2].m_texCoord[0] = 1.0f;
		texQuadVerts[2].m_texCoord[1] = 1.0f;

		texQuadVerts[3].m_position[0] = -1.0f;
		texQuadVerts[3].m_position[1] = 1.0f;
		texQuadVerts[3].m_position[2] = 0.5f;
		texQuadVerts[3].m_texCoord[0] = 0.0f;
		texQuadVerts[3].m_texCoord[1] = 1.0f;

		std::vector<GLuint> texQuadIndices;
		texQuadIndices.resize(6);
		texQuadIndices[0] = 0;
		texQuadIndices[1] = 1;
		texQuadIndices[2] = 2;
		texQuadIndices[3] = 0;
		texQuadIndices[4] = 2;
		texQuadIndices[5] = 3;

		std::vector<Attribute> texQuadAttributes;
		texQuadAttributes.resize(2);

		texQuadAttributes[0].m_name = "v_position";
		texQuadAttributes[0].m_numFloats = 3;
		texQuadAttributes[0].m_floatOffset = 0;

		texQuadAttributes[1].m_name = "v_texCoord";
		texQuadAttributes[1].m_numFloats = 2;
		texQuadAttributes[1].m_floatOffset = 3;

		m_localAudioSource.m_pTexQuad = new StaticMesh<TexQuadVert>(m_openGLContext, texQuadVerts, texQuadIndices, texQuadAttributes);
		m_remoteAudioSource.m_pTexQuad = new StaticMesh<TexQuadVert>(m_openGLContext, texQuadVerts, texQuadIndices, texQuadAttributes);
		m_combinedAudioSource.m_pTexQuad = new StaticMesh<TexQuadVert>(m_openGLContext, texQuadVerts, texQuadIndices, texQuadAttributes);
	}

	KickFaceAudioProcessor* pLocalProcessor = m_localAudioSource.m_processor.get();
	if(pLocalProcessor)
	{
		m_localAudioSource.m_prevCache.m_beatBufferPosition = pLocalProcessor->getBeatBufferPosition();
		m_localAudioSource.m_prevCache.m_delaySamples = (float)pLocalProcessor->getDelayValue().getValue();
	}

	KickFaceAudioProcessor* pRemoteProcessor = m_remoteAudioSource.m_processor.get();
	if(pRemoteProcessor)
	{
		m_remoteAudioSource.m_prevCache.m_beatBufferPosition = pRemoteProcessor->getBeatBufferPosition();
		m_remoteAudioSource.m_prevCache.m_delaySamples = (float)pRemoteProcessor->getDelayValue().getValue();
	}

	m_localAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
	m_remoteAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
	m_combinedAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
	m_resizeImages = false;
}


void AudioDisplayComponent::renderOpenGL()
{
	jassert(OpenGLHelpers::isContextActive());

	// initialise opengl if needed
	initialiseOpenGL();

	// resize images if needed
	if(m_resizeImages && m_pQuadMeshShaderProgram != nullptr)
	{
		m_localAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
		m_remoteAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
		m_combinedAudioSource.m_image = Image(Image::ARGB, AUDIODISPLAY_UPSCALE * getWidth(), AUDIODISPLAY_UPSCALE * getHeight(), true, OpenGLImageType());
		m_resizeImages = false;
	}

	// render waveforms to render targets
	const float desktopScale = (float)m_openGLContext.getRenderingScale();
	OpenGLHelpers::clear(Colour::greyLevel(0.1f));

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glEnable(GL_POLYGON_SMOOTH);

	glViewport(0, 0, roundToInt(AUDIODISPLAY_UPSCALE * getWidth()), roundToInt(AUDIODISPLAY_UPSCALE * getHeight()));

	m_pQuadMeshShaderProgram->useProgram();

	std::array<float, 4> colour;
	colour[0] = 0.3f;
	colour[1] = 0.3f;
	colour[2] = 0.3f;
	colour[3] = 1.0f;
	renderCombinedAudioSource(m_combinedAudioSource, colour);
	
	colour[0] = 141.0f / 255.0f;
	colour[1] = 21.0f / 255.0f;
	colour[2] = 74.0f / 255.0f;
	colour[3] = 1.0f;
	renderAudioSource(m_localAudioSource, colour);

	colour[0] = 40.0f / 255.0f;
	colour[1] = 119.0f / 255.0f;
	colour[2] = 118.0f / 255.0f;
	colour[3] = 1.0f;
	renderAudioSource(m_remoteAudioSource, colour);

	glViewport(0, 0, roundToInt(desktopScale * getWidth()), roundToInt(desktopScale * getHeight()));

	// render waveform textures
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	m_pTexQuadShaderProgram->useProgram();

	// render audio sources
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// render local audio sources
	OpenGLFrameBuffer* pLocalTex = OpenGLImageType::getFrameBufferFrom(m_localAudioSource.m_image);
	m_openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pLocalTex->getTextureID());
	m_openGLContext.extensions.glUniform1i(m_pTexQuadShaderProgram->getUniformIndex("texture0"), 0);
	m_localAudioSource.m_pTexQuad->draw(m_pTexQuadShaderProgram);

	// render remote audio source
	if(m_remoteAudioSource.m_processor.get())
	{
		OpenGLFrameBuffer* pRemoteTex = OpenGLImageType::getFrameBufferFrom(m_remoteAudioSource.m_image);
		m_openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pRemoteTex->getTextureID());
		m_openGLContext.extensions.glUniform1i(m_pTexQuadShaderProgram->getUniformIndex("texture0"), 0);
		m_remoteAudioSource.m_pTexQuad->draw(m_pTexQuadShaderProgram);

		// render combined
		OpenGLFrameBuffer* pCombinedTex = OpenGLImageType::getFrameBufferFrom(m_combinedAudioSource.m_image);
		m_openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pCombinedTex->getTextureID());
		m_openGLContext.extensions.glUniform1i(m_pTexQuadShaderProgram->getUniformIndex("texture0"), 0);
		m_combinedAudioSource.m_pTexQuad->draw(m_pTexQuadShaderProgram);
	}

	// render time bars	
	m_pQuadMeshShaderProgram->useProgram();

	int numBars = 0;
	const float timeBarHalfWidth = 1.0f / getWidth();
	for(int barIndex = 0; barIndex < 3; ++barIndex)
	{
		float xPos = (0.25f * (barIndex + 1) - m_viewStartRatio) / (m_viewEndRatio - m_viewStartRatio);
		if(xPos + timeBarHalfWidth > 0.0f || xPos - timeBarHalfWidth < 1.0f)
		{
			std::array<ColQuadVert, 4> verts;

			verts[0].m_position[0] = 2.0f * jmax(xPos - timeBarHalfWidth, 0.0f) - 1.0f;
			verts[0].m_position[1] = 1.0f;
			verts[0].m_position[2] = 0.1f;

			verts[1].m_position[0] = 2.0f * jmin(xPos + timeBarHalfWidth, 1.0f) - 1.0f;
			verts[1].m_position[1] = 1.0f;
			verts[1].m_position[2] = 0.1f;

			verts[2].m_position[0] = 2.0f * jmin(xPos + timeBarHalfWidth, 1.0f) - 1.0f;
			verts[2].m_position[1] = -1.0f;
			verts[2].m_position[2] = 0.1f;

			verts[3].m_position[0] = 2.0f * jmax(xPos - timeBarHalfWidth, 0.0f) - 1.0f;
			verts[3].m_position[1] = -1.0f;
			verts[3].m_position[2] = 0.1f;

			for(int i = 0; i < 4; ++i)
			{
				verts[i].m_colour[0] = 0.2f;
				verts[i].m_colour[1] = 0.2f;
				verts[i].m_colour[2] = 0.2f;
				verts[i].m_colour[3] = 1.0f;
			}

			m_pQuadMesh->setQuad(numBars, verts);
			++numBars;
		}
	}

	if(numBars > 0)
		m_pQuadMesh->draw(m_pQuadMeshShaderProgram, 0, numBars - 1);
}


void AudioDisplayComponent::renderAudioSource(AudioSource& audioSource, const std::array<float, 4>& colour)
{
	KickFaceAudioProcessor* pProcessor = audioSource.m_processor.get();
	if(!pProcessor)
		return;

	OpenGLFrameBuffer* pRenderTarget = OpenGLImageType::getFrameBufferFrom(audioSource.m_image);
	pRenderTarget->makeCurrentAndClear();

	int64 nextBeatBufferPosition = pProcessor->getBeatBufferPosition();
	int nextDelaySamples = (float)pProcessor->getDelayValue().getValue();
	bool nextInvertPhase = (float)pProcessor->getInvertPhaseValue().getValue() > 0.5f;
	int nextListenMode = roundFloatToInt((float)pProcessor->getListenModeValue().getValue());

	// update mesh
	AudioSampleBuffer* pBeatBuffer = pProcessor->getBeatBuffer();
	if(pBeatBuffer && pBeatBuffer->getNumSamples() > 0)
	{
		const int numBeatSamples = pBeatBuffer->getNumSamples();
		const float sampleScale = (float)numBeatSamples / AUDIODISPLAY_NUM_QUADS;
		const int beatBufferChangeSampleStart = Math::positiveModulo(audioSource.m_prevCache.m_beatBufferPosition + nextDelaySamples - (int)ceilf(sampleScale), numBeatSamples);
		const int beatBufferChangeSampleEnd = Math::positiveModulo(nextBeatBufferPosition + nextDelaySamples + (int)ceilf(sampleScale), numBeatSamples);

		const int viewStartBeatSample = (int)floorf(m_viewStartRatio * numBeatSamples);
		const int viewEndBeatSample = (int)ceilf(m_viewEndRatio * numBeatSamples);
		const float viewScale = (float)(viewEndBeatSample - viewStartBeatSample) / AUDIODISPLAY_NUM_QUADS;

		int startQuad = 0;
		int endQuad = AUDIODISPLAY_NUM_QUADS - 1;
		const float sampleSign = nextInvertPhase ? -1.0f : 1.0f;
		const float* pReadBuffer = pBeatBuffer->getReadPointer(0);

		const float kVertDepth = 0.5f;
		float vertXScale = 2.0f / AUDIODISPLAY_NUM_QUADS;
		float vertXPos = -1.0f;
		float vertYScale = 1.0f;
		float vertYPos = 0.0f;
		std::array<ColQuadVert, 4> verts;

		float startSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + (startQuad * viewScale) - nextDelaySamples);
		float endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((startQuad + 1) * viewScale) - nextDelaySamples);
		for(int i = startQuad; i <= endQuad; ++i)
		{
			verts[0].m_position[0] = vertXPos + ((i - 1) * vertXScale);
			verts[0].m_position[1] = jlimit(-1.0f, 1.0f, vertYPos + (startSample * vertYScale));
			verts[0].m_position[2] = kVertDepth;

			verts[1].m_position[0] = vertXPos + (i * vertXScale);
			verts[1].m_position[1] = jlimit(-1.0f, 1.0f, vertYPos + (endSample * vertYScale));
			verts[1].m_position[2] = kVertDepth;

			verts[2].m_position[0] = vertXPos + (i * vertXScale);
			verts[2].m_position[1] = vertYPos;
			verts[2].m_position[2] = kVertDepth;

			verts[3].m_position[0] = vertXPos + ((i - 1) * vertXScale);
			verts[3].m_position[1] = vertYPos;
			verts[3].m_position[2] = kVertDepth;

			for(int j = 0; j < 4; ++j)
			{
				verts[j].m_colour[0] = colour[0];
				verts[j].m_colour[1] = colour[1];
				verts[j].m_colour[2] = colour[2];
				verts[j].m_colour[3] = colour[3];
			}

			audioSource.m_pQuadMesh->setQuad(i, verts);

			startSample = endSample;
			endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((i + 2) * viewScale) - nextDelaySamples);
		}
	}

	// render
	audioSource.m_pQuadMesh->draw(m_pQuadMeshShaderProgram);
	pRenderTarget->releaseAsRenderingTarget();

	// cache prev values
	audioSource.m_prevCache.m_beatBufferPosition = nextBeatBufferPosition;
	audioSource.m_prevCache.m_delaySamples = nextDelaySamples;
	audioSource.m_prevCache.m_invertPhase = nextInvertPhase;
	audioSource.m_prevCache.m_listenMode = nextListenMode;
}


void AudioDisplayComponent::renderCombinedAudioSource(CombinedAudioSource& combinedSource, const std::array<float, 4>& colour)
{
	OpenGLFrameBuffer* pRenderTarget = OpenGLImageType::getFrameBufferFrom(combinedSource.m_image);
	pRenderTarget->makeCurrentAndClear();

	m_renderAudioSources.clear();
	for(int i = 0; i < combinedSource.m_audioSources.size(); ++i)
	{
		AudioSource* pAudioSource = combinedSource.m_audioSources[i];
		if(pAudioSource && pAudioSource->m_processor.get() && pAudioSource->m_processor->getBeatBuffer() && pAudioSource->m_processor->getBeatBuffer()->getNumSamples() > 0)
		{
			RenderAudioSource renderSource;
			renderSource.m_pAudioSource = pAudioSource;
			renderSource.m_pBeatBuffer = pAudioSource->m_processor->getBeatBuffer();
			renderSource.m_nextCache.m_beatBufferPosition = pAudioSource->m_processor->getBeatBufferPosition();
			renderSource.m_nextCache.m_delaySamples = (float)pAudioSource->m_processor->getDelayValue().getValue();
			renderSource.m_nextCache.m_invertPhase = (float)pAudioSource->m_processor->getInvertPhaseValue().getValue() > 0.5f;
			renderSource.m_nextCache.m_listenMode = roundFloatToInt((float)pAudioSource->m_processor->getListenModeValue().getValue());
			m_renderAudioSources.push_back(renderSource);
		}
	}

	if(m_renderAudioSources.size() == 0)
		return;

	// update mesh
	const int numBeatSamples = m_renderAudioSources[0].m_pBeatBuffer->getNumSamples();
	if(numBeatSamples > 0)
	{
		const float sampleScale = (float)numBeatSamples / AUDIODISPLAY_NUM_QUADS;

		int beatBufferChangeSampleStart = Math::positiveModulo(m_renderAudioSources[0].m_pAudioSource->m_prevCache.m_beatBufferPosition + m_renderAudioSources[0].m_nextCache.m_delaySamples - (int)ceilf(sampleScale), numBeatSamples);
		int beatBufferChangeSampleEnd = Math::positiveModulo(m_renderAudioSources[0].m_nextCache.m_beatBufferPosition + m_renderAudioSources[0].m_nextCache.m_delaySamples + (int)ceilf(sampleScale), numBeatSamples);
		for(int i = 1; i < m_renderAudioSources.size(); ++i)
		{
			beatBufferChangeSampleStart = jmin(beatBufferChangeSampleStart, Math::positiveModulo(m_renderAudioSources[i].m_pAudioSource->m_prevCache.m_beatBufferPosition + m_renderAudioSources[i].m_nextCache.m_delaySamples - (int)ceilf(sampleScale), numBeatSamples));
			beatBufferChangeSampleEnd = jmax(beatBufferChangeSampleEnd, Math::positiveModulo(m_renderAudioSources[i].m_nextCache.m_beatBufferPosition + m_renderAudioSources[i].m_nextCache.m_delaySamples + (int)ceilf(sampleScale), numBeatSamples));
		}

		const int viewStartBeatSample = (int)floorf(m_viewStartRatio * numBeatSamples);
		const int viewEndBeatSample = (int)ceilf(m_viewEndRatio * numBeatSamples);
		const float viewScale = (float)(viewEndBeatSample - viewStartBeatSample) / AUDIODISPLAY_NUM_QUADS;

		int startQuad = 0;
		int endQuad = AUDIODISPLAY_NUM_QUADS - 1;

		for(int i = 0; i < m_renderAudioSources.size(); ++i)
		{
			m_renderAudioSources[i].m_sampleSign = m_renderAudioSources[i].m_nextCache.m_invertPhase ? -1.0f : 1.0f;
			m_renderAudioSources[i].m_pReadBuffer = m_renderAudioSources[i].m_pBeatBuffer->getReadPointer(0);
		}

		const float kVertDepth = 0.5f;
		float vertXScale = 2.0f / AUDIODISPLAY_NUM_QUADS;
		float vertXPos = -1.0f;
		float vertYScale = 1.0f;
		float vertYPos = 0.0f;
		std::array<ColQuadVert, 4> verts;

		float startSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + (startQuad * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
		float endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((startQuad + 1) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
		for(int i = 1; i < m_renderAudioSources.size(); ++i)
		{
			startSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + (startQuad * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((startQuad + 1) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
		}

		for(int q = startQuad; q <= endQuad; ++q)
		{
			verts[0].m_position[0] = vertXPos + ((q - 1) * vertXScale);
			verts[0].m_position[1] = jlimit(-1.0f, 1.0f, vertYPos + (startSample * vertYScale));
			verts[0].m_position[2] = kVertDepth;

			verts[1].m_position[0] = vertXPos + (q * vertXScale);
			verts[1].m_position[1] = jlimit(-1.0f, 1.0f, vertYPos + (endSample * vertYScale));
			verts[1].m_position[2] = kVertDepth;

			verts[2].m_position[0] = vertXPos + (q * vertXScale);
			verts[2].m_position[1] = vertYPos;
			verts[2].m_position[2] = kVertDepth;

			verts[3].m_position[0] = vertXPos + ((q - 1) * vertXScale);
			verts[3].m_position[1] = vertYPos;
			verts[3].m_position[2] = kVertDepth;

			for(int j = 0; j < 4; ++j)
			{
				verts[j].m_colour[0] = colour[0];
				verts[j].m_colour[1] = colour[1];
				verts[j].m_colour[2] = colour[2];
				verts[j].m_colour[3] = colour[3];
			}

			combinedSource.m_pQuadMesh->setQuad(q, verts);

			startSample = endSample;
			endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
			for(int i = 1; i < m_renderAudioSources.size(); ++i)
				endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
		}
	}
	
	// render
	m_combinedAudioSource.m_pQuadMesh->draw(m_pQuadMeshShaderProgram);
	pRenderTarget->releaseAsRenderingTarget();
}


void AudioDisplayComponent::openGLContextClosing()
{
	m_localAudioSource.m_pQuadMesh = nullptr;
	m_remoteAudioSource.m_pQuadMesh = nullptr;
	m_combinedAudioSource.m_pQuadMesh = nullptr;
	m_localAudioSource.m_image = Image::null;
	m_remoteAudioSource.m_image = Image::null;
	m_combinedAudioSource.m_pQuadMesh = nullptr;
	m_pQuadMesh = nullptr;
	m_pQuadMeshShaderProgram = nullptr;
}


void AudioDisplayComponent::paint(Graphics& g)
{
}


void AudioDisplayComponent::resized() 
{
	m_resizeImages = true;
}


float AudioDisplayComponent::sampleBuffer(const float* pReadBuffer, int bufferSize, float samplePosition) const
{
	if(bufferSize > 0)
	{
		int sampleIndex = (int)floorf(samplePosition);
		float sampleRatio = samplePosition - sampleIndex;
		return (pReadBuffer[Math::positiveModulo(sampleIndex, bufferSize)] * (1.0f - sampleRatio))
			+ (pReadBuffer[Math::positiveModulo(sampleIndex + 1, bufferSize)] * sampleRatio);
	}

#if USE_LOGGING
	Logger::writeToLog(String("AudioDisplayComponent::sampleBuffer - buffer size zero"));
#endif
	return 0.0f;
}


void AudioDisplayComponent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if(m_dragMode != E_DragMode::None)
		return;

	const float zoomSpeed = 0.1f;
	float nextZoomLevel = jlimit(0.01f, 1.0f, m_zoomLevel + wheel.deltaY * zoomSpeed);

	if(nextZoomLevel == m_zoomLevel)
		return;

	m_zoomLevel = nextZoomLevel;

	float viewRatioWidth = m_viewEndRatio - m_viewStartRatio;
	float mouseRatio = event.position.x / getWidth();
	float focusPosition = m_viewStartRatio + mouseRatio * viewRatioWidth;

	viewRatioWidth = 1.0f - m_zoomLevel;
	viewRatioWidth = viewRatioWidth * viewRatioWidth * viewRatioWidth;

	m_viewStartRatio = focusPosition - mouseRatio * viewRatioWidth;
	m_viewEndRatio = m_viewStartRatio + viewRatioWidth;

	if(m_viewStartRatio < 0.0f)
	{
		m_viewEndRatio -= m_viewStartRatio;
		m_viewStartRatio = 0.0f;
	}
	else if(m_viewEndRatio > 1.0f)
	{
		m_viewStartRatio -= m_viewEndRatio - 1.0f;
		m_viewEndRatio = 1.0f;
	}

	m_viewStartRatio = jlimit(0.0f, 1.0f, m_viewStartRatio);
	m_viewEndRatio = jlimit(0.0f, 1.0f, m_viewEndRatio);
}


void AudioDisplayComponent::mouseDown(const MouseEvent& event)
{
	m_dragMode = juce::ModifierKeys::getCurrentModifiers().isShiftDown() ? E_DragMode::Local : 
		juce::ModifierKeys::getCurrentModifiers().isAltDown() ? E_DragMode::Remote : E_DragMode::View;
	m_dragSamples = 0;
	m_dragViewStart = m_viewStartRatio;
}


void AudioDisplayComponent::mouseUp(const MouseEvent& event)
{
	m_dragMode = E_DragMode::None;
	m_dragSamples = 0;
}


void AudioDisplayComponent::mouseDrag(const MouseEvent& event)
{
	KickFaceAudioProcessor* pProcessor = m_localAudioSource.m_processor.get();
	if(pProcessor)
	{
		float mouseRatio = event.getDistanceFromDragStartX() / getWidth();
		float viewWidth = m_viewEndRatio - m_viewStartRatio;
		float dragDistanceRatio = event.getDistanceFromDragStartX() * viewWidth / getWidth();

		switch(m_dragMode)
		{
		case E_DragMode::View:
			{
				m_viewStartRatio = jlimit(0.0f, 1.0f - viewWidth, m_dragViewStart - dragDistanceRatio);
				m_viewEndRatio = m_viewStartRatio + viewWidth;
			}
			break;

		case E_DragMode::Local:
			{
				int nextDragSamples = roundFloatToInt(dragDistanceRatio * pProcessor->getBeatBuffer()->getNumSamples());
				int currentDelaySamples = roundFloatToInt((float)pProcessor->getDelayValue().getValue());
				int nextDelaySamples = jlimit(-SAMPLE_DELAY_RANGE, SAMPLE_DELAY_RANGE, currentDelaySamples + nextDragSamples - m_dragSamples);
				pProcessor->getDelayValue().setValue((float)nextDelaySamples);
				m_dragSamples = nextDragSamples;
			}
			break;

		case E_DragMode::Remote:
			{
				KickFaceAudioProcessor* pRemoteProcessor = m_remoteAudioSource.m_processor.get();
				if(pRemoteProcessor)
				{
					int nextDragSamples = roundFloatToInt(dragDistanceRatio * pRemoteProcessor->getBeatBuffer()->getNumSamples());
					int currentDelaySamples = roundFloatToInt((float)pRemoteProcessor->getDelayValue().getValue());
					int nextDelaySamples = jlimit(-SAMPLE_DELAY_RANGE, SAMPLE_DELAY_RANGE, currentDelaySamples + nextDragSamples - m_dragSamples);
					pRemoteProcessor->getDelayValue().setValue((float)nextDelaySamples);
					m_dragSamples = nextDragSamples;
				}
			}
			break;
		}
	}
}