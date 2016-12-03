/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_92F4C47D3CC9BDB0__
#define __JUCE_HEADER_92F4C47D3CC9BDB0__

//[Headers]     -- You can add your own extra header files here --
#include "JuceHeader.h"
//[/Headers]



//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Introjucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class sharedMemoryShowComponent  : public Component
{
public:
    //==============================================================================
    sharedMemoryShowComponent ();
    ~sharedMemoryShowComponent();

    //==============================================================================
    //[UserMethods]     -- You can add your own custom methods in this section.
    //[/UserMethods]

    void paint (Graphics& g);
    void resized();

	void setVideoInfo(String name, int w, int h, int bufs);
	void setAudioInfo(String name, int chs, int freq, int blockSize, int bufs);

private:
    //[UserVariables]   -- You can add your own custom variables in this section.
    //[/UserVariables]

    //==============================================================================
    ScopedPointer<GroupComponent> m_groupVideo;
    ScopedPointer<GroupComponent> m_groupAudio;
    ScopedPointer<Label> label;
    ScopedPointer<Label> label2;
    ScopedPointer<Label> label3;
    ScopedPointer<Label> label4;
    ScopedPointer<TextEditor> m_txtVideoName;
    ScopedPointer<TextEditor> m_txtVideoWidth;
    ScopedPointer<TextEditor> m_txtVideoHeight;
    ScopedPointer<TextEditor> m_txtVideoBuffers;
    ScopedPointer<Label> label5;
    ScopedPointer<Label> label6;
    ScopedPointer<Label> label7;
    ScopedPointer<Label> label8;
    ScopedPointer<TextEditor> m_txtAudioName;
    ScopedPointer<TextEditor> m_txtAudioChannels;
    ScopedPointer<TextEditor> m_txtAudioFrequency;
    ScopedPointer<TextEditor> m_txtAudioBufferSize;
    ScopedPointer<Label> label9;
    ScopedPointer<TextEditor> m_txtAudioBuffers;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (sharedMemoryShowComponent)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

#endif   // __JUCE_HEADER_92F4C47D3CC9BDB0__
