# Guitar Plugin Rig Assets

These assets are copied into the standalone and VST3 build outputs.

## Amp NAM captures

- AC30 C2X: `Assets/NAM/VoxAC30`
- JCM800 Modded: `Assets/NAM/JCM800`
- Twin Reverb: `Assets/NAM/TwinReverb`

## Cab IRs

- AC30 Blue 2x12: `Assets/IR/VoxBlue212`
- Marshall 1960A 4x12 Greenbacks: `Assets/IR/Marshall1960A`
- Twin Reverb cab IR set: `Assets/IR/TwinReverb`

## Current state

The plugin UI can select these rig families and the build copies the assets into the outputs.
IR convolution is implemented with JUCE DSP.

NAM inference is wired through `Source/NamModel.*`. If `third_party/NeuralAmpModelerCore`
is present, CMake enables `GUITARPLUGIN_HAS_NAM` and compiles the NAM engine into the plugin.
If the engine is missing, the plugin builds with the NAM wrapper disabled and falls back to the
starter amp waveshaper.
