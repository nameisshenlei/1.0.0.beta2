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

#include "silentEncoderShowComponent.h"
#include "publicHeader.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
silentEncoderShowComponent::silentEncoderShowComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]


    //[UserPreSize]
	addAndMakeVisible(m_encoderProperty = new PropertyPanel());
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

silentEncoderShowComponent::~silentEncoderShowComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]



    //[Destructor]. You can add your own custom destruction code here..
	m_encoderProperty = nullptr;
    //[/Destructor]
}

//==============================================================================
void silentEncoderShowComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void silentEncoderShowComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]
	m_encoderProperty->setBounds(0, 0, getWidth(), getHeight());
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]

void silentEncoderShowComponent::onEncoderCreated(PropertySet* settingInfo)
{
	Array<PropertyComponent*> arrayComp;
	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_VIDEO_WIDTH)), L"width", 255, false));
	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_VIDEO_HEIGHT)), L"height", 255, false));
	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_VIDEO_BITRATE)), L"bitrate", 255, false));

	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_AUDIO_CHS)), L"channels", 255, false));
	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_AUDIO_FREQ)), L"frequency", 255, false));
	arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_AUDIO_BITRATE)), L"bitrate", 255, false));

	String strType = settingInfo->getValue(KEY_ENCODER_TYPE);
	if (strType.isEmpty())
		return;

	if (strType.equalsIgnoreCase(CMD_ENCODER_CREATE_TYPE_HARDWARE))
	{
		arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_HW_CARD)), L"hw card", 255, false));
		arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_HW_INDEX)), L"hw index", 255, false));
		arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_HW_KEYWORD)), L"hw keyword", 255, false));
	}
	else
	{
		arrayComp.add(new TextPropertyComponent(Value(settingInfo->getValue(KEY_OUT_URI)), L"uri", 255, false));
	}

	m_encoderProperty->addSection(
		settingInfo->getValue(KEY_ENCODER_NAME),
		arrayComp
		);
}

void silentEncoderShowComponent::onEncoderDestroyed(String strName)
{
	StringArray a = m_encoderProperty->getSectionNames();
	int i = a.indexOf(StringRef(strName));
	if (i >= 0)
	{
		m_encoderProperty->removeSection(i);
	}
}

//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="silentEncoderShowComponent"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ffffff"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
