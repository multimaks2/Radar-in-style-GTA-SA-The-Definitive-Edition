# Radar Trilogy SA
Custom radar for GTA San Andreas with extended features.

<img width="2560" height="1440" alt="Screenshot (250)" src="https://github.com/user-attachments/assets/7093eae1-857e-4ed0-a5d2-26e8416d7a5e" />

## Features
- Complete replacement of the default radar
- Displays radio station name
- Gang zone support
- Extended / advanced markers (based on the [More Icon](https://gist.github.com/JuniorDjjr/38ba16704a2de63fd7ea31711d6e9e0f) code example)
- Flexible configuration via ini file

## Installation
1. Copy `radar-trilogy-sa.asi` into your GTA SA `scripts` folder
2. Make sure you have [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) installed
3. Launch the game

## Requirements
- GTA San Andreas **v1.0 US**
- [Plugin SDK](https://github.com/DK22Pac/plugin-sdk) (for building)
- Visual Studio 2026

## Building
1. Clone the repository into `plugin-sdk\tools\myplugin-gen\generated\`
2. Set environment variable `PLUGIN_SDK_DIR` pointing to your Plugin SDK folder
3. Open `radar-trilogy-sa.sln` in Visual Studio 2026
4. Build the project in **Release** configuration

## Configuration
Config file: `radar-trilogy-sa.ini` (created automatically)

### Available settings
- `Shape`          — radar shape (0 = square, 1 = circle)
- `ShowGangZones`  — show gang zones (0 = off, 1 = on)
- `ModeMoreIcon`   — extended icons mode
- `CircleSize`     — radar size (when circle mode is active)

## License
MIT. See [LICENSE](LICENSE) file.

## Credits
- [DK22Pac](https://github.com/DK22Pac) — for Plugin SDK
- GTA modding community — for documentation and knowledge
- [JuniorDjjr](https://gist.github.com/JuniorDjjr) — for the More Icon example
