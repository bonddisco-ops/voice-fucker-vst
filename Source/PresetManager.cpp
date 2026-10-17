#include "PresetManager.h"
#include "Params.h"

namespace
{
    struct Factory { const char* name; std::vector<std::pair<juce::String, float>> values; };
    const std::vector<Factory>& factory()
    {
        using namespace ParamIDs;
        static const std::vector<Factory> list {
            { "Demon Choir", { { vocFormant, -6.f }, { vocBands, 32 }, { ivOn (0), 1 }, { ivOn (1), 1 }, { ivOn (7), 1 },
                               { dirtDrive, 45 }, { dirtCrunch, 55 }, { widthAmt, 70 }, { harmSpread, 50 } } },
            { "Daft Robot", { { vocBands, 16 }, { carSrc, 0 }, { carDetune, 10 }, { dirtDrive, 10 }, { dirtCrunch, 5 },
                              { vocRelease, 30 }, { widthAmt, 30 } } },
            { "Crunch Beast", { { vocBands, 24 }, { dirtMode, 4 }, { dirtDrive, 70 }, { dirtCrunch, 90 }, { dirtDestroy, 20 },
                                { vocFormant, -3 }, { ivOn (1), 1 }, { clipMode, 1 }, { compOn, 1 }, { ottDepth, 40 } } },
            { "Ghost Whisper", { { carSrc, 3 }, { vocBands, 40 }, { vocRelease, 120 }, { widthMode, 2 }, { widthAmt, 80 },
                                 { dirtDrive, 0 }, { dirtCrunch, 10 }, { vocSib, 70 } } },
            { "Goblin Squad", { { vocFormant, 7 }, { ivOn (5), 1 }, { ivOn (6), 1 }, { ivOn (8), 1 }, { harmSpread, 80 },
                                { dirtCrunch, 50 }, { dirtMode, 5 } } },
            { "Radio Grunt", { { vocBands, 8 }, { dirtMode, 5 }, { dirtDestroy, 60 }, { dirtCrunch, 70 }, { widthOn, 0 },
                               { vocFormant, -2 }, { filterId (true, fOn), 1 }, { filterId (true, fLc), 400 }, { filterId (true, fHc), 3500 } } },
            { "Wide Wall", { { vocBands, 48 }, { ivOn (1), 1 }, { ivOn (3), 1 }, { ivOn (7), 1 }, { ivOn (8), 1 }, { harmPan, 1 },
                             { harmSpread, 80 }, { widthAmt, 90 }, { dirtCrunch, 30 }, { compOn, 1 } } },
            { "Fuzz Preacher", { { vocOn, 0 }, { dirtMode, 3 }, { dirtCrunch, 55 }, { ivOn (1), 1 }, { ivOn (7), 1 },
                                 { compOn, 1 }, { compMode, 1 }, { widthMode, 2 } } },
            { "Parallel Robot", { { vocBands, 32 }, { dryGain, 0.f }, { wetGain, -5.f }, { filterId (true, fOn), 1 },
                                  { filterId (true, fTarget), 0 }, { filterId (true, fLc), 250 }, { filterId (true, fHc), 7000 },
                                  { dirtDrive, 40 }, { dirtCrunch, 40 } } },
            { "Harmony Only", { { vocOn, 0 }, { harmDry, Params::offDb }, { harmWet, 0.f }, { ivOn (1), 1 }, { ivOn (5), 1 },
                                { ivOn (7), 1 }, { dirtOn, 0 }, { dryGain, 0.f }, { wetGain, -3.f }, { harmSpread, 60 },
                                { filterId (false, fOn), 1 }, { filterId (false, fLc), 150 } } },
        };
        return list;
    }
}

juce::StringArray PresetManager::getFactoryNames()
{
    juce::StringArray s;
    for (auto& f : factory()) s.add (f.name);
    return s;
}

bool PresetManager::loadFactory (int index)
{
    if (index < 0 || index >= (int) factory().size()) return false;
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    for (auto& [id, v] : factory()[(size_t) index].values)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    currentName = factory()[(size_t) index].name;
    markClean();
    return true;
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (ranged->paramID, this);
}

PresetManager::~PresetManager()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (ranged->paramID, this);
}

juce::File PresetManager::getFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Primal Rage Lab").getChildFile ("Voice Fucker").getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

void PresetManager::parameterChanged (const juce::String&, float)
{
    if (juce::Time::getMillisecondCounter() > ignoreUntil.load())
        modified = true;
}

void PresetManager::markClean()
{
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    modified = false;
}

void PresetManager::loadInit()
{
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    currentName = "Init";
    markClean();
}

bool PresetManager::load (const juce::String& name)
{
    const auto file = getFolder().getChildFile (name + extension);
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    // Start from defaults so parameters missing in older presets are reset
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    auto tree = juce::ValueTree::fromXml (*xml);
    Params::upgradeState (tree);
    apvts.replaceState (tree);
    currentName = name;
    markClean();
    return true;
}

bool PresetManager::save (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;

    auto state = apvts.copyState();
    state.removeProperty ("presetName", nullptr);
    if (auto xml = state.createXml())
    {
        if (xml->writeTo (getFolder().getChildFile (clean + extension)))
        {
            currentName = clean;
            markClean();
            return true;
        }
    }
    return false;
}

bool PresetManager::remove (const juce::String& name)
{
    const bool ok = getFolder().getChildFile (name + extension).deleteFile();
    if (ok && name == currentName)
    {
        currentName = "Init";
        modified = true;
    }
    return ok;
}

bool PresetManager::rename (const juce::String& from, const juce::String& to)
{
    const auto clean = juce::File::createLegalFileName (to.trim());
    if (clean.isEmpty())
        return false;
    const auto src = getFolder().getChildFile (from + extension);
    const auto dst = getFolder().getChildFile (clean + extension);
    if (! src.existsAsFile() || dst.exists())
        return false;
    const bool ok = src.moveFileTo (dst);
    if (ok && from == currentName)
        currentName = clean;
    return ok;
}

void PresetManager::step (int direction)
{
    auto names = getFactoryNames();
    const int nf = names.size();
    names.addArray (getPresetNames());
    if (names.isEmpty())
        return;
    int idx = names.indexOf (currentName);
    if (idx < 0)
        idx = direction > 0 ? -1 : 0;
    idx = (idx + direction + names.size()) % names.size();
    if (idx < nf) loadFactory (idx);
    else load (names[idx]);
}
