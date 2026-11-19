#include "HeaderBar.h"

#include "../PluginProcessor.h"
#include "../Presets.h"
#include "RateMath.h"

using namespace juce;

namespace
{
/// German number format: 2.000 / 33,33
String de (double v, int decimals)
{
    String s = String (std::abs (v), decimals);
    String intPart = s.upToFirstOccurrenceOf (".", false, false);
    String frac = s.fromFirstOccurrenceOf (".", false, false);
    String grouped;
    for (int i = 0; i < intPart.length(); ++i)
    {
        if (i > 0 && (intPart.length() - i) % 3 == 0) grouped << ".";
        grouped << intPart[i];
    }
    return (v < 0 ? "-" : "") + grouped + (frac.isNotEmpty() ? "," + frac : String());
}

const char* const kDivNames[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
} // namespace

HeaderBar::HeaderBar (XtoneProcessor& p) : proc (p)
{
    for (auto* b : { &loadButton, &normButton, &savePresetButton, &loadPresetButton }) addAndMakeVisible (*b);
    addAndMakeVisible (presetBox);
    for (auto* l : { &fileLabel, &statusLabel, &liveLabel }) addAndMakeVisible (*l);
    liveLabel.setFont (FontOptions (15.0f, Font::bold));
    statusLabel.setColour (Label::textColourId, Colours::orange);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> ("Sample laden", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this] (const FileChooser& fc)
                              {
                                  if (fc.getResult().existsAsFile()) proc.loadSampleFile (fc.getResult());
                              });
    };
    normButton.onClick = [this] { proc.normalizeSample(); };

    savePresetButton.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> ("Preset speichern", File(), "*.xtone");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
                              [this] (const FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f == File()) return;
                                  proc.saveStateToFile (f.withFileExtension ("xtone"));
                              });
    };
    loadPresetButton.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> ("Preset laden", File(), "*.xtone");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [this] (const FileChooser& fc)
                              {
                                  if (fc.getResult().existsAsFile()) proc.loadStateFromFile (fc.getResult());
                              });
    };

    refreshPresets();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getCurrentProgram()) proc.setCurrentProgram (idx);
    };
    startTimerHz (30);
}

void HeaderBar::refreshPresets()
{
    presetBox.clear (dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), dontSendNotification);
}

void HeaderBar::resized()
{
    auto r = getLocalBounds().reduced (4);
    auto top = r.removeFromTop (28);
    loadButton.setBounds (top.removeFromLeft (80));
    top.removeFromLeft (4);
    normButton.setBounds (top.removeFromLeft (110));
    top.removeFromLeft (8);
    loadPresetButton.setBounds (top.removeFromRight (120));
    top.removeFromRight (4);
    savePresetButton.setBounds (top.removeFromRight (100));
    top.removeFromRight (4);
    presetBox.setBounds (top.removeFromRight (180));
    top.removeFromRight (8);
    fileLabel.setBounds (top);
    r.removeFromTop (2);
    statusLabel.setBounds (r.removeFromBottom (18));
    liveLabel.setBounds (r);
}

void HeaderBar::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker (0.3f));
}

String HeaderBar::buildLiveText() const
{
    const auto& t = proc.telemetry();
    auto raw = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const int src = (int) raw ("rate_src");
    const int div = jlimit (0, 6, (int) raw ("division"));
    const int mod = (int) raw ("div_mod");
    const String divText = String (kDivNames[div]) + (mod == 1 ? "T" : mod == 2 ? "." : "");

    double hz = t.rateHz.load();
    const bool active = t.active.load();
    if (! active)
    {
        // idle: show what the root note would play
        if (src == 0) hz = xt::bpmToHz (raw ("bpm"), div, mod);
        else if (src == 1) hz = raw ("hz");
        else hz = 0.0;
    }

    String s;
    const String dot = String::fromUTF8 (" · ");
    if (hz > 0.0)
    {
        const double note = xt::hzToNote (hz, raw ("a4_ref"));
        s << "Rate " << de (hz, 2) << " Hz" << dot << de (xt::hzToBpmEquivalent (hz), 0) << String::fromUTF8 (" BPM-Äq.") << dot;
        if (src == 0) s << "Plugin " << de (raw ("bpm"), 0) << " BPM @" << divText << dot;
        else if (src == 1) s << "Hz-Modus" << dot;
        else s << String::fromUTF8 ("Host ×") << (1 << (int) raw ("host_mult")) << " @" << divText << dot;
        s << String (xt::noteName (note, false)) << " (FL " << String (xt::noteName (note, true)) << ")";
    }
    else
        s << "Host-Tempo" << dot << divText;
    s << dot << "Slice " << (t.slice.load() + 1) << "/" << t.numSlices.load();
    if (! active) s << "  [idle]";
    return s;
}

void HeaderBar::timerCallback()
{
    liveLabel.setText (buildLiveText(), dontSendNotification);
    if (auto a = proc.getCurrentAsset())
        fileLabel.setText (a->name + (a->normalized ? " (normalisiert)" : "") + (a->fileBytes != nullptr ? " [eingebettet]" : ""),
                           dontSendNotification);
    statusLabel.setText (proc.getStatusMessage(), dontSendNotification);
    if (presetBox.getSelectedItemIndex() != proc.getCurrentProgram())
        presetBox.setSelectedItemIndex (proc.getCurrentProgram(), dontSendNotification);
}
