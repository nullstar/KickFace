#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>



class KickFaceAudioProcessor;



class GlobalProcessorArray
{
public:
	class Listener
	{
	public:
		virtual ~Listener() { masterReference.clear(); }
		virtual void processorAdded(KickFaceAudioProcessor* pProcessor) {}
		virtual void processorRemoved(KickFaceAudioProcessor* pProcessor) {}
		virtual void processorGivenNameChanged(KickFaceAudioProcessor* pProcessor) {}

		WeakReference<GlobalProcessorArray::Listener>::Master masterReference;
		friend class WeakReference<GlobalProcessorArray::Listener>;
	};

	static void addProcessor(KickFaceAudioProcessor* pProcessor);
	static void removeProcessor(KickFaceAudioProcessor* pProcessor);
	static void processorGivenNameChanged(KickFaceAudioProcessor* pProcessor);

	static const std::vector<WeakReference<KickFaceAudioProcessor>>& getProcessors();
	static KickFaceAudioProcessor* getProcessorById(int id);

	static void addListener(Listener* pListener);
	static void removeListener(Listener* pListener);

private:
	static std::vector<WeakReference<KickFaceAudioProcessor>> s_processorArray;
	static std::vector<WeakReference<Listener>> s_listeners;
};