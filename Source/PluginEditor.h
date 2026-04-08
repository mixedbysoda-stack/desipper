#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SipperLookAndFeel.h"
#include "UI/PluginPanel.h"
#include "Licensing/ActivationDialog.h"

class ResizeHandle : public juce::Component
{
public:
    ResizeHandle() { setMouseCursor (juce::MouseCursor::BottomRightCornerResizeCursor); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff52525b));

        // Three diagonal lines (grip dots pattern)
        for (int i = 0; i < 3; ++i)
        {
            float offset = float (i) * 5.0f;
            g.drawLine (b.getRight() - offset, b.getBottom(),
                        b.getRight(), b.getBottom() - offset, 1.5f);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStart = e.getEventRelativeTo (getParentComponent()).getPosition();
        parentSizeAtDrag = getParentComponent()->getLocalBounds().getBottomRight();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        auto pos = e.getEventRelativeTo (getParentComponent()).getPosition();
        auto delta = pos - dragStart;
        auto* parent = getParentComponent();
        int newW = juce::jlimit (600, 1400, parentSizeAtDrag.x + delta.x);
        int newH = juce::jlimit (420, 980, parentSizeAtDrag.y + delta.y);
        parent->setSize (newW, newH);
    }

private:
    juce::Point<int> dragStart;
    juce::Point<int> parentSizeAtDrag;
};

class SipperEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit SipperEditor (SipperProcessor& p);
    ~SipperEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    SipperProcessor& processor;
    SipperLookAndFeel lookAndFeel;
    PluginPanel panel;
    ActivationDialog activationDialog;
    ResizeHandle resizeHandle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SipperEditor)
};
