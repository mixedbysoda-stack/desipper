#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// LCD numeric readout: black inset background, glowing red monospace text
// Supports click+drag for value adjustment when draggable is enabled
class LCDDisplay : public juce::Component
{
public:
    LCDDisplay (const juce::String& labelText = {});

    void setValue (const juce::String& text);
    void setDraggable (float minVal, float maxVal, float step);
    float getNumericValue() const { return numericValue; }

    void paint (juce::Graphics& g) override;
    void resized() override {}

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    std::function<void (float)> onValueChanged;

private:
    juce::String label;
    juce::String value = "0.0";

    // Drag state
    bool isDraggable = false;
    float numericValue = 0.0f;
    float dragMin = 0.0f;
    float dragMax = 1.0f;
    float dragStep = 1.0f;
    float dragStartValue = 0.0f;
    int dragStartY = 0;
};
