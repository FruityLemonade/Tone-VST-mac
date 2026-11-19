#include "PluginEditor.h"

#include "../PluginProcessor.h"

using namespace juce;

XtoneEditor::XtoneEditor (XtoneProcessor& p)
    : AudioProcessorEditor (p), proc (p), header (p), waveform (p), params (p, p.apvts)
{
    addAndMakeVisible (header);
    addAndMakeVisible (waveform);
    addAndMakeVisible (params);
    setResizable (true, true);
    setResizeLimits (760, 560, 2400, 1800);
    setSize (1000, 720);
}

void XtoneEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void XtoneEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (78));
    waveform.setBounds (r.removeFromTop (200).reduced (4, 2));
    params.setBounds (r.reduced (4));
}

bool XtoneEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
    return false;
}

void XtoneEditor::filesDropped (const StringArray& files, int, int)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3"))
        {
            proc.loadSampleFile (File (f));
            return;
        }
}
