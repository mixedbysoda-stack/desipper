#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Color palette matching Magic Patterns SIPPER design
namespace SipperColors
{
    const juce::Colour background      { 0xff2a2b2e };  // Dark charcoal/gunmetal
    const juce::Colour panelBg         { 0xff333437 };  // Slightly lighter panel
    const juce::Colour insetBg         { 0xff0a0a0a };  // Deep black for LCD/meters
    const juce::Colour headerGradStart { 0xff3f3f46 };  // zinc-700
    const juce::Colour headerGradEnd   { 0xff27272a };  // zinc-800
    const juce::Colour borderLight     { 0xff52525b };  // zinc-600
    const juce::Colour borderDark      { 0xff18181b };  // zinc-900
    const juce::Colour textBright      { 0xffd4d4d8 };  // zinc-300
    const juce::Colour textDim         { 0xffa1a1aa };  // zinc-400
    const juce::Colour lcdRed          { 0xffef4444 };  // red-500 (LCD text)
    const juce::Colour meterBlue       { 0xff3b82f6 };  // blue-500 (threshold)
    const juce::Colour meterRed        { 0xffef4444 };  // red-500 (attenuation top)
    const juce::Colour meterOrange     { 0xfff97316 };  // orange-500 (attenuation bottom)
    const juce::Colour meterAmber      { 0xfff59e0b };  // amber-500 (output)
    const juce::Colour buttonFace      { 0xff52525b };  // zinc-600
    const juce::Colour buttonFaceDark  { 0xff3f3f46 };  // zinc-700
    const juce::Colour ledYellow       { 0xfffacc15 };  // yellow-400 (active LED)
    const juce::Colour screwLight      { 0xffa1a1aa };  // zinc-400
    const juce::Colour screwDark       { 0xff52525b };  // zinc-600
}

class SipperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SipperLookAndFeel();

    juce::Font getLCDFont (float height) const;
    juce::Font getLabelFont (float height) const;
    juce::Font getTitleFont (float height) const;

    // Draw helpers matching the skeuomorphic style
    static void drawInsetPanel (juce::Graphics& g, juce::Rectangle<float> bounds);
    static void drawOutsetPanel (juce::Graphics& g, juce::Rectangle<float> bounds);
    static void drawScrew (juce::Graphics& g, juce::Point<float> center, float size, float rotation);
    static void drawBrushedMetalOverlay (juce::Graphics& g, juce::Rectangle<int> bounds);
};
