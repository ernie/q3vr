#!/bin/sh
# Downloads official Quake 3 artwork from Steam CDN for SteamVR launcher icons.
# Run this from your Q3VR install directory.

CDN="https://steamcdn-a.akamaihd.net/steam/apps"

echo "Downloading Quake III Arena artwork..."
curl -sL "$CDN/2200/header.jpg" -o q3a_header.jpg
curl -sL "$CDN/2200/logo.png" -o q3a_logo.png
curl -sL "$CDN/2200/library_hero_2x.jpg" -o q3a_library_hero.jpg
curl -sL "$CDN/2200/library_600x900_2x.jpg" -o q3a_library_600x900.jpg

echo "Downloading Quake III: Team Arena artwork..."
curl -sL "$CDN/2350/header.jpg" -o q3ta_header.jpg
curl -sL "$CDN/2350/logo.png" -o q3ta_logo.png
curl -sL "$CDN/2350/library_hero_2x.jpg" -o q3ta_library_hero.jpg
curl -sL "$CDN/2350/library_600x900_2x.jpg" -o q3ta_library_600x900.jpg

echo ""
echo "Done! Header images are used automatically by q3vr.vrmanifest."
echo ""
echo "To set the remaining artwork in Steam:"
echo "  1. Add Q3VR as a non-Steam game in your Steam library"
echo "  2. Name it \"Quake III Arena\" or \"Quake III: Team Arena\" accordingly"
echo "  3. Right-click the game's artwork in the library"
echo "  4. Set Custom Logo    -> q3a_logo.png (or q3ta_logo.png)"
echo "  5. Set Custom Background -> q3a_library_hero.jpg (or q3ta_library_hero.jpg)"
echo ""
echo "  For the library grid image, right-click the large icon in the"
echo "  library and select Manage -> Set Custom Artwork -> q3a_library_600x900.jpg"
echo "  (or q3ta_library_600x900.jpg)"
