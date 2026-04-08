#include "PluginEditor.h"

SipperEditor::SipperEditor (SipperProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      panel (p.getAPVTS(), p.getEngine()),
      activationDialog (p.getLicenseManager())
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);
    addChildComponent (activationDialog);
    addAndMakeVisible (resizeHandle);

    setSize (800, 560);
    setResizable (true, false); // resizable but no native corner

    // Show activation dialog if not yet activated
    if (! processor.getLicenseManager().isActivated())
        activationDialog.setVisible (true);

    startTimerHz (4);
}

SipperEditor::~SipperEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SipperEditor::paint (juce::Graphics&)
{
}

void SipperEditor::resized()
{
    panel.setBounds (getLocalBounds());
    activationDialog.setBounds (getLocalBounds());

    // Resize handle in bottom-right corner
    int handleSize = 18;
    resizeHandle.setBounds (getWidth() - handleSize, getHeight() - handleSize,
                            handleSize, handleSize);
    resizeHandle.toFront (false);
}

void SipperEditor::timerCallback()
{
    if (processor.getLicenseManager().isActivated() && activationDialog.isVisible())
        activationDialog.setVisible (false);
}
