#include "GlobalProcessorArray.h"
#include "PluginProcessor.h"


std::vector<WeakReference<KickFaceAudioProcessor>> GlobalProcessorArray::s_processorArray;
std::vector<WeakReference<GlobalProcessorArray::Listener>> GlobalProcessorArray::s_listeners;


void GlobalProcessorArray::addProcessor(KickFaceAudioProcessor* pProcessor)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::addProcessor"));
#endif

	if(pProcessor == nullptr)
		return;

	for(int i = 0; i < s_processorArray.size(); ++i)
		if(s_processorArray[i].get() == pProcessor)
			return;

	s_processorArray.push_back(pProcessor);
	for(int i = 0; i < s_listeners.size(); ++i)
	{
		Listener* pListener = s_listeners[i].get();
		if(pListener)
			pListener->processorAdded(pProcessor);
	}
}


void GlobalProcessorArray::removeProcessor(KickFaceAudioProcessor* pProcessor)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::removeProcessor"));
#endif

	if(pProcessor == nullptr)
		return;

	bool removed = false;
	for(int i = 0; i < s_processorArray.size(); ++i)
	{
		if(s_processorArray[i].get() == pProcessor)
		{
			s_processorArray[i] = s_processorArray[s_processorArray.size() - 1];
			s_processorArray.pop_back();
			--i;
			removed = true;
		}
	}

	if(removed)
	{
		for(int i = 0; i < s_listeners.size(); ++i)
		{
			Listener* pListener = s_listeners[i].get();
			if(pListener)
				pListener->processorRemoved(pProcessor);
		}
	}
}


void GlobalProcessorArray::processorGivenNameChanged(KickFaceAudioProcessor* pProcessor)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::processorGivenNameChanged"));
#endif

	for(int i = 0; i < s_listeners.size(); ++i)
	{
		Listener* pListener = s_listeners[i].get();
		if(pListener)
			pListener->processorGivenNameChanged(pProcessor);
	}
}


const std::vector<WeakReference<KickFaceAudioProcessor>>& GlobalProcessorArray::getProcessors()
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::getProcessors"));
#endif

	return s_processorArray;
}


KickFaceAudioProcessor* GlobalProcessorArray::getProcessorById(int id)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::getProcessorById"));
#endif

	for(int i = 0; i < s_processorArray.size(); ++i)
	{
		KickFaceAudioProcessor* pProcessor = s_processorArray[i].get();
		if(pProcessor && pProcessor->getInstanceId() == id)
			return pProcessor;
	}
	return nullptr;
}


void GlobalProcessorArray::addListener(Listener* pListener)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::addListener"));
#endif

	if(pListener == nullptr)
		return;

	for(int i = 0; i < s_listeners.size(); ++i)
		if(s_listeners[i].get() == pListener)
			return;

	s_listeners.push_back(pListener);
}


void GlobalProcessorArray::removeListener(Listener* pListener)
{
#if USE_LOGGING
	Logger::writeToLog(String("GlobalProcessorArray::removeListener"));
#endif

	if(pListener == nullptr)
		return;

	for(int i = 0; i < s_listeners.size(); ++i)
	{
		if(s_listeners[i].get() == pListener)
		{
			s_listeners[i] = s_listeners[s_listeners.size() - 1];
			s_listeners.pop_back();
			--i;
		}
	}
}