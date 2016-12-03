/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "encoderManagerListener.h"

class sharedMemoryShowComponent;
class silentEncoderShowComponent;

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component
	, public nemoEncoderManagerListener
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&);
    void resized();

	virtual void onVideoSharedMemoryCreate(String name, int w, int h, int bufs) override;
	virtual void onVideoSharedMemoryDestroyed() override;
	virtual void onAudioSharedMemoryCreated(String name, int chs, int freq, int blockSize, int bufs) override;
	virtual void onAudioSharedMemoryDestroyed() override;

	virtual void onEncoderCreated(PropertySet* settingInfo) override;
	virtual void onEncoderDestroyed(String strName) override;
private:
	ScopedPointer<TabbedComponent>						m_tabsShow;
	ScopedPointer<sharedMemoryShowComponent>			m_tabSharedMemory;
	ScopedPointer<silentEncoderShowComponent>			m_tabSilentEncoder;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
