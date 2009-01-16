#!/bin/sh

notify-send "Hello" "Hello World"
sleep 2
notify-send " " -i $HOME/src/alsdorf/icons/volume.svg -h int:value:75
sleep 2
notify-send " " -i $HOME/src/alsdorf/icons/volume.svg -h int:value:30
sleep 2
notify-send " " -i $HOME/src/alsdorf/icons/brightness.svg -h int:value:85
sleep 2
notify-send " " -i $HOME/src/alsdorf/icons/brightness.svg -h int:value:45
sleep 2
notify-send "WiFi signal found" -i $HOME/src/alsdorf/icons/wifi-75.svg
sleep 2
notify-send "WiFi signal lost" -i $HOME/src/alsdorf/icons/wifi-lost.svg
sleep 2
notify-send "English bubble" "The quick brown fox jumps over the lazy dog." -i /usr/share/icons/Human/scalable/places/network.svg
sleep 2
notify-send "Bubble from Germany" "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark." -i /usr/share/icons/Human/scalable/categories/gnome-system.svg
sleep 2
notify-send "Very russian" "Съешь ещё этих мягких французских булок, да выпей чаю." -i /usr/share/icons/Human/scalable/status/dialog-info.svg
sleep 2
notify-send "More from Germany" "Oje, Qualm verwölkt Dix zig Farbtriptychons." -i /usr/share/icons/Human/scalable/categories/gnome-globe.svg
