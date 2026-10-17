#!/bin/bash
# Voice Fucker installer for macOS: copies the plugins and removes the download quarantine.
cd "$(dirname "$0")"
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
echo "Installing Voice Fucker..."
rm -rf "$VST3/Voice Fucker.vst3" "$AU/Voice Fucker.component"
cp -R "Voice Fucker.vst3" "$VST3/" && echo "  VST3 -> $VST3"
cp -R "Voice Fucker.component" "$AU/" && echo "  AU   -> $AU"
xattr -cr "$VST3/Voice Fucker.vst3" "$AU/Voice Fucker.component" 2>/dev/null
xattr -cr "Voice Fucker.app" 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
echo ""
echo "Done! Rescan plugins in your DAW. / Готово! Пересканируй плагины в DAW."
echo "You can close this window. / Это окно можно закрыть."
