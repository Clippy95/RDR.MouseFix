# RDR.MouseFix
This is a simple fix to mouse getting clamped while in cover.

script comes in .asi format.
# Installation: 
The mod IS NOT reliant on any ScriptHook/RedHook.

Drag and Drop to game's directory, ensuring you have some type of an [ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) installed, 

Otherwise this works with Redhook with the `AllowASIPluginsLoading=true` option set in RedHook.ini

- Clamping of X-axis's mouse while in cover is automatically patched out.
# INI config:


- FixMouseCoverSensitivity (Applies the cover_sens_fix_multiplier to mouse X delta while in cover.)
- FixInvertOptionForMouseCover (Fixes invert option ingame not applying to cover camera.) false by default.
- cover_sens_fix_multiplier (Multiplier for X-axis delta while in cover, it feels halved, 2x is a good default.)
