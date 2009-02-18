#!/bin/sh

notify-send "WiFi signal found" -i /usr/share/icons/Human/scalable/status/notification-network-wireless-medium.svg
sleep 2
notify-send "WiFi signal lost" -i /usr/share/icons/Human/scalable/status/notification-network-wireless-disconnected.svg
sleep 2
notify-send " " -i /usr/share/icons/Human/scalable/status/notification-audio-volume-medium.svg -h int:value:75 -h string:synchronous:volume
sleep 2
notify-send " " -i /usr/share/icons/Human/scalable/status/notification-audio-volume-low.svg -h int:value:30 -h string:synchronous:volume
sleep 2
notify-send " " -i /usr/share/icons/Human/scalable/status/notification-display-brightness-high.svg -h int:value:100 -h string:synchronous:brightness
sleep 2
notify-send " " -i /usr/share/icons/Human/scalable/status/notification-display-brightness-medium.svg -h int:value:45 -h string:synchronous:brightness
sleep 2
notify-send "Testing markup" "Some <b>bold</b>, <u>underlined</u>, <i>italic</i> text"
sleep 2
notify-send "Jamshed Kakar" "Hey, what about this restaurant? http://www.blafasel.org

Would you go from your place by train or should I pick you up from work? What do you think?"
sleep 2
notify-send "English bubble" "The quick brown fox jumps over the lazy dog." -i /usr/share/icons/Human/scalable/places/network.svg
sleep 2
notify-send "Bubble from Germany" "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark." -i /usr/share/icons/Human/scalable/categories/gnome-system.svg
sleep 2
notify-send "Very russian" "Съешь ещё этих мягких французских булок, да выпей чаю." -i /usr/share/icons/Human/scalable/status/dialog-info.svg
sleep 2
notify-send "More from Germany" "Oje, Qualm verwölkt Dix zig Farbtriptychons." -i /usr/share/icons/Human/scalable/categories/gnome-globe.svg


