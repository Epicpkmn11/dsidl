# DSi Downloader
[![Build status on GitHub Actions](https://github.com/Epicpkmn11/dsidl/actions/workflows/build.yml/badge.svg)](https://github.com/Epicpkmn11/dsidl/actions/workflows/build.yml)
[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-908a85?logo=gitpod)](https://www.gitpod.io)

A simple app to download a file from a URL in a QR code on the Nintendo DSi.

![Screenshot of the main menu](/resources/screenshots/main-menu.png)


## Usage
1. Add a Wi-Fi network in System Settings under "Advanced Settings"
   - Due to compatibility issues in the dsiwifi library not all routers will work, WEP/open routers also likely don't work at all
2. Open dsidl and wait for it to load, if it gets stuck you can reload it while holding <kbd>SELECT</kbd> to view more detailed logging information
3. Create a QR code for what you want to download and scan it with your DSi
   - The QR code must contain a URL or [JSON script](https://github.com/Epicpkmn11/dsidl/wiki/Scripting), if not either of those the text will simply be printed out
5. Choose the directory to save to, edit the file name if needed, and your file will start downloading
6. You're done!

## Building
1. Install [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started)
2. Run:
   ```bash
   sudo dkp-pacman -S nds-dev nds-zlib
   ```
   (On Windows it's `pacman` instead of `sudo dkp-pacman`, command may vary on Linux)
3. Run:
   ```bash
   make
   ```

## Credits
- [dsiwifi](https://github.com/shinyquagsire23/dsiwifi): DSi Wi-Fi library
- [dsi-camera](https://github.com/Epicpkmn11/dsi-camera): Camera code base
- [Universal-Updater](https://github.com/Universal-Team/Universal-Updater): Download code base
- [GodMode9i](https://github.com/DS-Homebrew/GodMode9i): Base for several UI elements; keyboard, file browser, etc
- [devkitPro](https://devkitpro.org): libnds, devkitARM, libfat
