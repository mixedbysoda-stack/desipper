#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// 3D beveled toggle button with optional LED indicator
// Matches Magic Patterns: skeuo-button, active state with yellow-400 LED
class SkeuoButton : public juce::Component
{
public:
    SkeuoButton (const juce::String& text);

    void setToggled (bool toggled);
    bool isToggled() const { return active; }
    void setClickCallback (std::function<void()> cb) { onClick = std::move (cb); }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::String label;
    bool active = false;
    std::function<void()> onClick;
};
