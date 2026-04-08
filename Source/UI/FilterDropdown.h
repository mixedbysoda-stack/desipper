#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// Filter mode dropdown matching Magic Patterns: outset panel with waveform icon + dropdown arrow
class FilterDropdown : public juce::Component
{
public:
    FilterDropdown();

    void setIsBandPass (bool bp);
    bool getIsBandPass() const { return isBandPass; }
    void setClickCallback (std::function<void()> cb) { onClick = std::move (cb); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    bool isBandPass = false;
    std::function<void()> onClick;

    void drawFilterIcon (juce::Graphics& g, juce::Rectangle<float> area);
};
