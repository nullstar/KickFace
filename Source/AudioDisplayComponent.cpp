#include "AudioDisplayComponent.h"
#include "PluginProcessor.h"
#include "Math.h"


#define AUDIODISPLAY_NEAR 0.0f
#define AUDIODISPLAY_FAR 1.0f
#define AUDIODISPLAY_NUM_QUADS 500
#define AUDIODISPLAY_UPSCALE 1



const char* gQuadMeshVertexShaderSource = 
"in vec3 v_position;\
in vec4 v_colour;\
\
out vec4 f_colour;\
\
void main()\
{\
	gl_Position = vec4(v_position, 1.0f);\
	f_colour = v_colour;\
}";


const char* gQuadMeshFragmentShaderSource = 
"in vec4 f_colour;\
\
out vec4 out_colour;\
\
void main()\
{\
	out_colour = f_colour;\
}";



const char* gTexQuadVertexShaderSource =
"in vec3 v_position;\
in vec2 v_texCoord;\
\
out vec2 f_texCoord;\
\
void main()\
{\
	gl_Position = vec4(v_position, 1.0f);\
	f_texCoord = v_texCoord;\
}";


const char* gTexQuadFragmentShaderSource =
"in vec2 f_texCoord;\
\
out vec4 out_colour;\
\
uniform sampler2D texture0;\
\
void main()\
{\
	out_colour = texture2D(texture0, f_texCoord);\
}";




AudioDisplayComponent::AudioDisplayComponent(KickFaceAudioProcessor& processor)
	: m_refresh(true)
	, m_viewStartRatio(0.0f)
	, m_viewEndRatio(1.0f)
	, m_zoomLevel(0.0f)
	, m_dragMode(E_DragMode::None)
	, m_dragSamples(0)
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
	m_refresh = true;
}


void AudioDisplayComponent::newOpenGLContextCreated()
{
	m_pQuadMeshShaderProgram = new ShaderProgram(m_openGLContext);
	if(m_pQuadMeshShaderProgram->load(gQuadMeshVertexShaderSource, gQuadMeshFragmentShaderSource))
	{
		int positionAttributeIndex = m_pQuadMeshShaderProgram->getAttributeIndex("v_position");
		int colourAttributeIndex = m_pQuadMeshShaderProgram->getAttributeIndex("v_colour");

		m_pQuadMesh = new QuadMesh(m_openGLContext);
		m_pQuadMesh->initialise(positionAttributeIndex, colourAttributeIndex, 3, 3);

		m_localAudioSource.m_pQuadMesh = new FixedQuadMesh(m_openGLContext);
		m_localAudioSource.m_pQuadMesh->initialise(positionAttributeIndex, colourAttributeIndex, AUDIODISPLAY_NUM_QUADS, 3);

		m_remoteAudioSource.m_pQuadMesh = new FixedQuadMesh(m_openGLContext);
		m_remoteAudioSource.m_pQuadMesh->initialise(positionAttributeIndex, colourAttributeIndex, AUDIODISPLAY_NUM_QUADS, 3);

		m_combinedAudioSource.m_pQuadMesh = new FixedQuadMesh(m_openGLContext);
		m_combinedAudioSource.m_pQuadMesh->initialise(positionAttributeIndex, colourAttributeIndex, AUDIODISPLAY_NUM_QUADS, 3);
	}

	m_pTexQuadShaderProgram = new ShaderProgram(m_openGLContext);
	if(m_pTexQuadShaderProgram->load(gTexQuadVertexShaderSource, gTexQuadFragmentShaderSource))
	{
		int positionAttributeIndex = m_pTexQuadShaderProgram->getAttributeIndex("v_position");
		int texAttributeIndex = m_pTexQuadShaderProgram->getAttributeIndex("v_texCoord");

		std::array<float, 2> topLeft;
		topLeft[0] = -1.0f;
		topLeft[1] = -1.0f;

		std::array<float, 2> dimensions;
		dimensions[0] = 2.0f;
		dimensions[1] = 2.0f;

		m_localAudioSource.m_pTexQuad = new StaticTexQuad(m_openGLContext);
		m_localAudioSource.m_pTexQuad->initialise(positionAttributeIndex, texAttributeIndex, topLeft, dimensions, 0.5f);

		m_remoteAudioSource.m_pTexQuad = new StaticTexQuad(m_openGLContext);
		m_remoteAudioSource.m_pTexQuad->initialise(positionAttributeIndex, texAttributeIndex, topLeft, dimensions, 0.5f);

		m_combinedAudioSource.m_pTexQuad = new StaticTexQuad(m_openGLContext);
		m_combinedAudioSource.m_pTexQuad->initialise(positionAttributeIndex, texAttributeIndex, topLeft, dimensions, 0.5f);
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

	m_refresh = true;
}


void AudioDisplayComponent::renderOpenGL()
{
	// render waveforms to render targets
	jassert(OpenGLHelpers::isContextActive());

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
	renderCombinedAudioSource(m_combinedAudioSource, m_refresh, colour);
	
	colour[0] = 141.0f / 255.0f;
	colour[1] = 21.0f / 255.0f;
	colour[2] = 74.0f / 255.0f;
	colour[3] = 1.0f;
	renderAudioSource(m_localAudioSource, m_refresh, colour);

	colour[0] = 40.0f / 255.0f;
	colour[1] = 119.0f / 255.0f;
	colour[2] = 118.0f / 255.0f;
	colour[3] = 1.0f;
	renderAudioSource(m_remoteAudioSource, m_refresh, colour);

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
	m_localAudioSource.m_pTexQuad->render();

	// render remote audio source
	if(m_remoteAudioSource.m_processor.get())
	{
		OpenGLFrameBuffer* pRemoteTex = OpenGLImageType::getFrameBufferFrom(m_remoteAudioSource.m_image);
		m_openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pRemoteTex->getTextureID());
		m_openGLContext.extensions.glUniform1i(m_pTexQuadShaderProgram->getUniformIndex("texture0"), 0);
		m_remoteAudioSource.m_pTexQuad->render();

		// render combined
		OpenGLFrameBuffer* pCombinedTex = OpenGLImageType::getFrameBufferFrom(m_combinedAudioSource.m_image);
		m_openGLContext.extensions.glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pCombinedTex->getTextureID());
		m_openGLContext.extensions.glUniform1i(m_pTexQuadShaderProgram->getUniformIndex("texture0"), 0);
		m_combinedAudioSource.m_pTexQuad->render();
	}

	// render time bars	
	m_pQuadMeshShaderProgram->useProgram();

	const float timeBarHalfWidth = 0.003f;
	for(int barIndex = 0; barIndex < 3; ++barIndex)
	{
		float xPos = (0.25f * (barIndex + 1) - m_viewStartRatio) / (m_viewEndRatio - m_viewStartRatio);
		if(xPos + timeBarHalfWidth > 0.0f || xPos - timeBarHalfWidth < 1.0f)
		{
			std::array<QuadMesh::QuadVert, 4> verts;

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

			m_pQuadMesh->add(verts);
		}
	}
	m_pQuadMesh->render();

	// reset refresh
	m_refresh = false;
}


void AudioDisplayComponent::renderAudioSource(AudioSource& audioSource, bool refresh, const std::array<float, 4>& colour)
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

	// update refresh status
	if(audioSource.m_prevCache.m_delaySamples != nextDelaySamples 
		|| audioSource.m_prevCache.m_invertPhase != nextInvertPhase 
		|| audioSource.m_prevCache.m_listenMode != nextListenMode)
		refresh = true;

	// update mesh
	AudioSampleBuffer* pBeatBuffer = pProcessor->getBeatBuffer();
	if(pBeatBuffer)
	{
		const int numBeatSamples = pBeatBuffer->getNumSamples();
		const float sampleScale = (float)numBeatSamples / AUDIODISPLAY_NUM_QUADS;
		const int beatBufferChangeSampleStart = Math::positiveModulo(audioSource.m_prevCache.m_beatBufferPosition + nextDelaySamples - (int)ceilf(sampleScale), numBeatSamples);
		const int beatBufferChangeSampleEnd = Math::positiveModulo(nextBeatBufferPosition + nextDelaySamples + (int)ceilf(sampleScale), numBeatSamples);

		const int viewStartBeatSample = (int)floorf(m_viewStartRatio * numBeatSamples);
		const int viewEndBeatSample = (int)ceilf(m_viewEndRatio * numBeatSamples);
		if(refresh || !(beatBufferChangeSampleEnd < viewStartBeatSample || beatBufferChangeSampleStart > viewEndBeatSample))
		{
			const float viewScale = (float)(viewEndBeatSample - viewStartBeatSample) / AUDIODISPLAY_NUM_QUADS;

			int startQuad = 0;
			int endQuad = AUDIODISPLAY_NUM_QUADS - 1;
			if(!refresh)
			{
				startQuad = (int)floorf(jmax(beatBufferChangeSampleStart - viewStartBeatSample, 0) / viewScale);
				endQuad = (int)ceilf(jmin(beatBufferChangeSampleEnd - viewStartBeatSample, AUDIODISPLAY_NUM_QUADS) / viewScale);
			}

			const float sampleSign = nextInvertPhase ? -1.0f : 1.0f;
			const float* pReadBuffer = pBeatBuffer->getReadPointer(0);

			const float kVertDepth = 0.5f;
			float vertXScale = 2.0f / AUDIODISPLAY_NUM_QUADS;
			float vertXPos = -1.0f;
			float vertYScale = 1.0f;
			float vertYPos = 0.0f;
			std::array<FixedQuadMesh::QuadVert, 4> verts;

			if(startQuad <= endQuad)
			{
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

					audioSource.m_pQuadMesh->setQuad(verts, i);

					startSample = endSample;
					endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((i + 2) * viewScale) - nextDelaySamples);
				}
			}
			else
			{
				float startSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample - nextDelaySamples);
				float endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + viewScale - nextDelaySamples);
				for(int i = 0; i < startQuad; ++i)
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

					audioSource.m_pQuadMesh->setQuad(verts, i);

					startSample = endSample;
					endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((i + 2) * viewScale) - nextDelaySamples);
				}

				startSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + (endQuad * viewScale) - nextDelaySamples);
				endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((endQuad + 1) * viewScale) - nextDelaySamples);
				for(int i = endQuad; i < AUDIODISPLAY_NUM_QUADS; ++i)
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

					audioSource.m_pQuadMesh->setQuad(verts, i);

					startSample = endSample;
					endSample = sampleSign * sampleBuffer(pReadBuffer, numBeatSamples, viewStartBeatSample + ((i + 2) * viewScale) - nextDelaySamples);
				}
			}
		}
	}

	// render
	if(audioSource.m_pQuadMesh->getNumVertices() > 0)
		audioSource.m_pQuadMesh->render();

	pRenderTarget->releaseAsRenderingTarget();

	// cache prev values
	audioSource.m_prevCache.m_beatBufferPosition = nextBeatBufferPosition;
	audioSource.m_prevCache.m_delaySamples = nextDelaySamples;
	audioSource.m_prevCache.m_invertPhase = nextInvertPhase;
	audioSource.m_prevCache.m_listenMode = nextListenMode;
}


void AudioDisplayComponent::renderCombinedAudioSource(CombinedAudioSource& combinedSource, bool refresh, const std::array<float, 4>& colour)
{
	OpenGLFrameBuffer* pRenderTarget = OpenGLImageType::getFrameBufferFrom(combinedSource.m_image);
	pRenderTarget->makeCurrentAndClear();

	m_renderAudioSources.clear();
	for(int i = 0; i < combinedSource.m_audioSources.size(); ++i)
	{
		AudioSource* pAudioSource = combinedSource.m_audioSources[i];
		if(pAudioSource && pAudioSource->m_processor.get() && pAudioSource->m_processor->getBeatBuffer())
		{
			RenderAudioSource renderSource;
			renderSource.m_pAudioSource = pAudioSource;
			renderSource.m_pBeatBuffer = pAudioSource->m_processor->getBeatBuffer();
			renderSource.m_nextCache.m_beatBufferPosition = pAudioSource->m_processor->getBeatBufferPosition();
			renderSource.m_nextCache.m_delaySamples = (float)pAudioSource->m_processor->getDelayValue().getValue();
			renderSource.m_nextCache.m_invertPhase = (float)pAudioSource->m_processor->getInvertPhaseValue().getValue() > 0.5f;
			renderSource.m_nextCache.m_listenMode = roundFloatToInt((float)pAudioSource->m_processor->getListenModeValue().getValue());

			// update refresh status
			if(pAudioSource->m_prevCache.m_delaySamples != renderSource.m_nextCache.m_delaySamples
				|| pAudioSource->m_prevCache.m_invertPhase != renderSource.m_nextCache.m_invertPhase
				|| pAudioSource->m_prevCache.m_listenMode != renderSource.m_nextCache.m_listenMode)
				refresh = true;

			m_renderAudioSources.push_back(renderSource);
		}
	}

	if(m_renderAudioSources.size() == 0)
		return;

	// update mesh
	const int numBeatSamples = m_renderAudioSources[0].m_pBeatBuffer->getNumSamples();
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
	if(refresh || !(beatBufferChangeSampleEnd < viewStartBeatSample || beatBufferChangeSampleStart > viewEndBeatSample))
	{
		const float viewScale = (float)(viewEndBeatSample - viewStartBeatSample) / AUDIODISPLAY_NUM_QUADS;

		int startQuad = 0;
		int endQuad = AUDIODISPLAY_NUM_QUADS - 1;
		if(!refresh)
		{
			startQuad = (int)floorf(jmax(beatBufferChangeSampleStart - viewStartBeatSample, 0) / viewScale);
			endQuad = (int)ceilf(jmin(beatBufferChangeSampleEnd - viewStartBeatSample, AUDIODISPLAY_NUM_QUADS) / viewScale);
		}

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
		std::array<FixedQuadMesh::QuadVert, 4> verts;

		if(startQuad <= endQuad)
		{
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

				combinedSource.m_pQuadMesh->setQuad(verts, q);

				startSample = endSample;
				endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
				for(int i = 1; i < m_renderAudioSources.size(); ++i)
					endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			}
		}
		else
		{
			float startSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample - m_renderAudioSources[0].m_nextCache.m_delaySamples);
			float endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + viewScale - m_renderAudioSources[0].m_nextCache.m_delaySamples);
			for(int i = 1; i < m_renderAudioSources.size(); ++i)
			{
				startSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample - m_renderAudioSources[i].m_nextCache.m_delaySamples);
				endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + viewScale - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			}

			for(int q = 0; q < startQuad; ++q)
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

				m_combinedAudioSource.m_pQuadMesh->setQuad(verts, q);

				startSample = endSample;
				endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
				for(int i = 1; i < m_renderAudioSources.size(); ++i)
					endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			}

			startSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + (endQuad * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
			endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((endQuad + 1) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
			for(int i = 1; i < m_renderAudioSources.size(); ++i)
			{
				startSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + (endQuad * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
				endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((endQuad + 1) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			}

			for(int q = endQuad; q < AUDIODISPLAY_NUM_QUADS; ++q)
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

				m_combinedAudioSource.m_pQuadMesh->setQuad(verts, q);

				startSample = endSample;
				endSample = m_renderAudioSources[0].m_sampleSign * sampleBuffer(m_renderAudioSources[0].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[0].m_nextCache.m_delaySamples);
				for(int i = 1; i < m_renderAudioSources.size(); ++i)
					endSample += m_renderAudioSources[i].m_sampleSign * sampleBuffer(m_renderAudioSources[i].m_pReadBuffer, numBeatSamples, viewStartBeatSample + ((q + 2) * viewScale) - m_renderAudioSources[i].m_nextCache.m_delaySamples);
			}
		}
	}
	
	// render
	if(m_combinedAudioSource.m_pQuadMesh->getNumVertices() > 0)
		m_combinedAudioSource.m_pQuadMesh->render();

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
	// This is called when the MainContentComponent is resized.
	// If you add any child components, this is where you should
	// update their positions.
}


float AudioDisplayComponent::sampleBuffer(const float* pReadBuffer, int bufferSize, float samplePosition) const
{
	int sampleIndex = (int)floorf(samplePosition);
	float sampleRatio = samplePosition - sampleIndex;
	return (pReadBuffer[Math::positiveModulo(sampleIndex, bufferSize)] * (1.0f - sampleRatio))
		+ (pReadBuffer[Math::positiveModulo(sampleIndex + 1, bufferSize)] * sampleRatio);
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
	m_refresh = true;

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

		m_refresh = true;
	}
}