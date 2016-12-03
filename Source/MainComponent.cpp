/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "sharedMemoryShowComponent.h"
#include "silentEncoderShowComponent.h"


//==============================================================================
MainContentComponent::MainContentComponent()
{
	addAndMakeVisible(m_tabsShow = new TabbedComponent(TabbedButtonBar::TabsAtTop));

	m_tabSharedMemory = new sharedMemoryShowComponent();
	m_tabsShow->addTab(L"¹²ÏíÄÚ´æ", Colours::lightgrey, m_tabSharedMemory, false);

	m_tabSilentEncoder = new silentEncoderShowComponent();
	m_tabsShow->addTab(L"Êä³ö", Colours::lightgrey, m_tabSilentEncoder, false);

    setSize (600, 400);
}

MainContentComponent::~MainContentComponent()
{
	m_tabSharedMemory = nullptr;
	m_tabSilentEncoder = nullptr;
	m_tabsShow = nullptr;
}

void MainContentComponent::paint (Graphics&/* g*/)
{
}

void MainContentComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.

	m_tabsShow->setBounds(0, 0, getWidth(), getHeight());
}

void MainContentComponent::onEncoderCreated(PropertySet* settingInfo)
{
	m_tabSilentEncoder->onEncoderCreated(settingInfo);
}

void MainContentComponent::onEncoderDestroyed(String strName)
{
	m_tabSilentEncoder->onEncoderDestroyed(strName);
}

void MainContentComponent::onVideoSharedMemoryCreate(String name, int w, int h, int bufs)
{
	m_tabSharedMemory->setVideoInfo(name, w, h, bufs);
}

void MainContentComponent::onVideoSharedMemoryDestroyed()
{
	m_tabSharedMemory->setVideoInfo(String::empty, 0, 0, 0);
}

void MainContentComponent::onAudioSharedMemoryCreated(String name, int chs, int freq, int blockSize, int bufs)
{
	m_tabSharedMemory->setAudioInfo(name, chs, freq, blockSize, bufs);
}

void MainContentComponent::onAudioSharedMemoryDestroyed()
{
	m_tabSharedMemory->setAudioInfo(String::empty, 0, 0, 0, 0);
}
