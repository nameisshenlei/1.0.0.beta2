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

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "sharedMemoryShowComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
sharedMemoryShowComponent::sharedMemoryShowComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (m_groupVideo = new GroupComponent ("new group",
                                                          TRANS("video")));

    addAndMakeVisible (m_groupAudio = new GroupComponent ("new group",
                                                          TRANS("audio")));

    addAndMakeVisible (label = new Label ("new label",
                                          TRANS("name")));
    label->setFont (Font (15.00f, Font::plain));
    label->setJustificationType (Justification::centredLeft);
    label->setEditable (false, false, false);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label2 = new Label ("new label",
                                           TRANS("width")));
    label2->setFont (Font (15.00f, Font::plain));
    label2->setJustificationType (Justification::centredLeft);
    label2->setEditable (false, false, false);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label3 = new Label ("new label",
                                           TRANS("height")));
    label3->setFont (Font (15.00f, Font::plain));
    label3->setJustificationType (Justification::centredLeft);
    label3->setEditable (false, false, false);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label4 = new Label ("new label",
                                           TRANS("buffers")));
    label4->setFont (Font (15.00f, Font::plain));
    label4->setJustificationType (Justification::centredLeft);
    label4->setEditable (false, false, false);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (m_txtVideoName = new TextEditor ("new text editor"));
    m_txtVideoName->setMultiLine (false);
    m_txtVideoName->setReturnKeyStartsNewLine (false);
    m_txtVideoName->setReadOnly (true);
    m_txtVideoName->setScrollbarsShown (false);
    m_txtVideoName->setCaretVisible (true);
    m_txtVideoName->setPopupMenuEnabled (true);
    m_txtVideoName->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtVideoName->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtVideoName->setText (String::empty);

    addAndMakeVisible (m_txtVideoWidth = new TextEditor ("new text editor"));
    m_txtVideoWidth->setMultiLine (false);
    m_txtVideoWidth->setReturnKeyStartsNewLine (false);
    m_txtVideoWidth->setReadOnly (true);
    m_txtVideoWidth->setScrollbarsShown (false);
    m_txtVideoWidth->setCaretVisible (false);
    m_txtVideoWidth->setPopupMenuEnabled (true);
    m_txtVideoWidth->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtVideoWidth->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtVideoWidth->setText (String::empty);

    addAndMakeVisible (m_txtVideoHeight = new TextEditor ("new text editor"));
    m_txtVideoHeight->setMultiLine (false);
    m_txtVideoHeight->setReturnKeyStartsNewLine (false);
    m_txtVideoHeight->setReadOnly (true);
    m_txtVideoHeight->setScrollbarsShown (false);
    m_txtVideoHeight->setCaretVisible (false);
    m_txtVideoHeight->setPopupMenuEnabled (true);
    m_txtVideoHeight->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtVideoHeight->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtVideoHeight->setText (String::empty);

    addAndMakeVisible (m_txtVideoBuffers = new TextEditor ("new text editor"));
    m_txtVideoBuffers->setMultiLine (false);
    m_txtVideoBuffers->setReturnKeyStartsNewLine (false);
    m_txtVideoBuffers->setReadOnly (true);
    m_txtVideoBuffers->setScrollbarsShown (false);
    m_txtVideoBuffers->setCaretVisible (false);
    m_txtVideoBuffers->setPopupMenuEnabled (true);
    m_txtVideoBuffers->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtVideoBuffers->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtVideoBuffers->setText (String::empty);

    addAndMakeVisible (label5 = new Label ("new label",
                                           TRANS("name")));
    label5->setFont (Font (15.00f, Font::plain));
    label5->setJustificationType (Justification::centredLeft);
    label5->setEditable (false, false, false);
    label5->setColour (TextEditor::textColourId, Colours::black);
    label5->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label6 = new Label ("new label",
                                           TRANS("channels")));
    label6->setFont (Font (15.00f, Font::plain));
    label6->setJustificationType (Justification::centredLeft);
    label6->setEditable (false, false, false);
    label6->setColour (TextEditor::textColourId, Colours::black);
    label6->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label7 = new Label ("new label",
                                           TRANS("frequency")));
    label7->setFont (Font (15.00f, Font::plain));
    label7->setJustificationType (Justification::centredLeft);
    label7->setEditable (false, false, false);
    label7->setColour (TextEditor::textColourId, Colours::black);
    label7->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (label8 = new Label ("new label",
                                           TRANS("bufferSize")));
    label8->setFont (Font (15.00f, Font::plain));
    label8->setJustificationType (Justification::centredLeft);
    label8->setEditable (false, false, false);
    label8->setColour (TextEditor::textColourId, Colours::black);
    label8->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (m_txtAudioName = new TextEditor ("new text editor"));
    m_txtAudioName->setMultiLine (false);
    m_txtAudioName->setReturnKeyStartsNewLine (false);
    m_txtAudioName->setReadOnly (true);
    m_txtAudioName->setScrollbarsShown (false);
    m_txtAudioName->setCaretVisible (false);
    m_txtAudioName->setPopupMenuEnabled (true);
    m_txtAudioName->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtAudioName->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtAudioName->setText (String::empty);

    addAndMakeVisible (m_txtAudioChannels = new TextEditor ("new text editor"));
    m_txtAudioChannels->setMultiLine (false);
    m_txtAudioChannels->setReturnKeyStartsNewLine (false);
    m_txtAudioChannels->setReadOnly (true);
    m_txtAudioChannels->setScrollbarsShown (false);
    m_txtAudioChannels->setCaretVisible (false);
    m_txtAudioChannels->setPopupMenuEnabled (true);
    m_txtAudioChannels->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtAudioChannels->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtAudioChannels->setText (String::empty);

    addAndMakeVisible (m_txtAudioFrequency = new TextEditor ("new text editor"));
    m_txtAudioFrequency->setMultiLine (false);
    m_txtAudioFrequency->setReturnKeyStartsNewLine (false);
    m_txtAudioFrequency->setReadOnly (true);
    m_txtAudioFrequency->setScrollbarsShown (false);
    m_txtAudioFrequency->setCaretVisible (false);
    m_txtAudioFrequency->setPopupMenuEnabled (true);
    m_txtAudioFrequency->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtAudioFrequency->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtAudioFrequency->setText (String::empty);

    addAndMakeVisible (m_txtAudioBufferSize = new TextEditor ("new text editor"));
    m_txtAudioBufferSize->setMultiLine (false);
    m_txtAudioBufferSize->setReturnKeyStartsNewLine (false);
    m_txtAudioBufferSize->setReadOnly (true);
    m_txtAudioBufferSize->setScrollbarsShown (false);
    m_txtAudioBufferSize->setCaretVisible (false);
    m_txtAudioBufferSize->setPopupMenuEnabled (true);
    m_txtAudioBufferSize->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtAudioBufferSize->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtAudioBufferSize->setText (String::empty);

    addAndMakeVisible (label9 = new Label ("new label",
                                           TRANS("buffers")));
    label9->setFont (Font (15.00f, Font::plain));
    label9->setJustificationType (Justification::centredLeft);
    label9->setEditable (false, false, false);
    label9->setColour (TextEditor::textColourId, Colours::black);
    label9->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (m_txtAudioBuffers = new TextEditor ("new text editor"));
    m_txtAudioBuffers->setMultiLine (false);
    m_txtAudioBuffers->setReturnKeyStartsNewLine (false);
    m_txtAudioBuffers->setReadOnly (true);
    m_txtAudioBuffers->setScrollbarsShown (false);
    m_txtAudioBuffers->setCaretVisible (false);
    m_txtAudioBuffers->setPopupMenuEnabled (true);
    m_txtAudioBuffers->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    m_txtAudioBuffers->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    m_txtAudioBuffers->setText (String::empty);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

sharedMemoryShowComponent::~sharedMemoryShowComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    m_groupVideo = nullptr;
    m_groupAudio = nullptr;
    label = nullptr;
    label2 = nullptr;
    label3 = nullptr;
    label4 = nullptr;
    m_txtVideoName = nullptr;
    m_txtVideoWidth = nullptr;
    m_txtVideoHeight = nullptr;
    m_txtVideoBuffers = nullptr;
    label5 = nullptr;
    label6 = nullptr;
    label7 = nullptr;
    label8 = nullptr;
    m_txtAudioName = nullptr;
    m_txtAudioChannels = nullptr;
    m_txtAudioFrequency = nullptr;
    m_txtAudioBufferSize = nullptr;
    label9 = nullptr;
    m_txtAudioBuffers = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void sharedMemoryShowComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void sharedMemoryShowComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    m_groupVideo->setBounds (0, 0, getWidth() - 0, 176);
    m_groupAudio->setBounds (0, 184, getWidth() - 0, 208);
    label->setBounds (16, 32, 72, 24);
    label2->setBounds (16, 64, 72, 24);
    label3->setBounds (16, 96, 72, 24);
    label4->setBounds (16, 128, 72, 24);
    m_txtVideoName->setBounds (96, 32, 264, 24);
    m_txtVideoWidth->setBounds (96, 64, 264, 24);
    m_txtVideoHeight->setBounds (96, 96, 264, 24);
    m_txtVideoBuffers->setBounds (96, 128, 264, 24);
    label5->setBounds (16, 216, 72, 24);
    label6->setBounds (16, 248, 72, 24);
    label7->setBounds (16, 280, 72, 24);
    label8->setBounds (16, 312, 72, 24);
    m_txtAudioName->setBounds (96, 216, 264, 24);
    m_txtAudioChannels->setBounds (96, 248, 264, 24);
    m_txtAudioFrequency->setBounds (96, 280, 264, 24);
    m_txtAudioBufferSize->setBounds (96, 312, 264, 24);
    label9->setBounds (16, 344, 72, 24);
    m_txtAudioBuffers->setBounds (96, 344, 264, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]

void sharedMemoryShowComponent::setVideoInfo(String name, int w, int h, int bufs)
{
	m_txtVideoName->setText(name, false);
	m_txtVideoWidth->setText(String(w), false);
	m_txtVideoHeight->setText(String(h), false);
	m_txtVideoBuffers->setText(String(bufs), false);
}

void sharedMemoryShowComponent::setAudioInfo(String name, int chs, int freq, int blockSize, int bufs)
{
	m_txtAudioName->setText(name, false);
	m_txtAudioChannels->setText(String(chs), false);
	m_txtAudioFrequency->setText(String(freq), false);
	m_txtAudioBufferSize->setText(String(blockSize), false);
	m_txtAudioBuffers->setText(String(bufs), false);
}
//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="sharedMemoryShowComponent"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ffffff"/>
  <GROUPCOMPONENT name="new group" id="fc9b9b422f2b9eb7" memberName="m_groupVideo"
                  virtualName="" explicitFocusOrder="0" pos="0 0 0M 176" title="video"/>
  <GROUPCOMPONENT name="new group" id="87cb4de3fc03f02a" memberName="m_groupAudio"
                  virtualName="" explicitFocusOrder="0" pos="0 184 0M 208" title="audio"/>
  <LABEL name="new label" id="eb2d5507748cd77b" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="16 32 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="name" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="9e8954c99e714e01" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="16 64 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="width" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="90ec668186dd79ab" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="16 96 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="height" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="31e4507f025d7ba3" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="16 128 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="buffers" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="c8251f8dc01bf20e" memberName="m_txtVideoName"
              virtualName="" explicitFocusOrder="0" pos="96 32 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="1" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="c6299dd549610b2" memberName="m_txtVideoWidth"
              virtualName="" explicitFocusOrder="0" pos="96 64 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="9a345bb8b05fde8d" memberName="m_txtVideoHeight"
              virtualName="" explicitFocusOrder="0" pos="96 96 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="d4439236ad601f89" memberName="m_txtVideoBuffers"
              virtualName="" explicitFocusOrder="0" pos="96 128 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <LABEL name="new label" id="a746be9f10b38483" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="16 216 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="name" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="b79068d17f420fb" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="16 248 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="channels" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="569bd5b254153ec7" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="16 280 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="frequency" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="512958086d51988c" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="16 312 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="bufferSize" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="237ef9b5e5ca3290" memberName="m_txtAudioName"
              virtualName="" explicitFocusOrder="0" pos="96 216 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="380d3945b4df6e0" memberName="m_txtAudioChannels"
              virtualName="" explicitFocusOrder="0" pos="96 248 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="fe2a41cdb9ae4dcf" memberName="m_txtAudioFrequency"
              virtualName="" explicitFocusOrder="0" pos="96 280 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <TEXTEDITOR name="new text editor" id="a7e9e25fce22b0f8" memberName="m_txtAudioBufferSize"
              virtualName="" explicitFocusOrder="0" pos="96 312 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
  <LABEL name="new label" id="bf65ad8c676c81e5" memberName="label9" virtualName=""
         explicitFocusOrder="0" pos="16 344 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="buffers" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="33"/>
  <TEXTEDITOR name="new text editor" id="e4574bd6db324949" memberName="m_txtAudioBuffers"
              virtualName="" explicitFocusOrder="0" pos="96 344 264 24" bkgcol="ffffff"
              shadowcol="0" initialText="" multiline="0" retKeyStartsLine="0"
              readonly="1" scrollbars="0" caret="0" popupmenu="1"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
