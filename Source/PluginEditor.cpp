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

void SipperEditor::parentHierarchyChanged()
{
    juce::AudioProcessorEditor::parentHierarchyChanged();

   #if JUCE_WINDOWS
    // Force JUCE 8's GDI software renderer instead of the new Direct2D backend.
    // Direct2D context init crashes on Intel Iris Xe integrated graphics inside
    // sandboxed plug-in host processes (Bitwig PluginHost, Reaper plug-in
    // scanner, MuLab) — same class of crash that took out Pour's demo on Win 11
    // Iris Xe rigs. Software renderer adds negligible CPU cost and is rock-solid
    // across GPU configs. Applied preemptively to De-Sipper so its Windows demo
    // never hits the same wall.
    if (auto* peer = getPeer())
    {
        const auto engines = peer->getAvailableRenderingEngines();
        const int softIdx = engines.indexOf ("Software Renderer");
        if (softIdx >= 0 && peer->getCurrentRenderingEngine() != softIdx)
            peer->setCurrentRenderingEngine (softIdx);
    }
   #endif
}
