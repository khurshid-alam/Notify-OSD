#!/bin/sh

notify-send "Take note" "The next example will test the icon-only layout-case" -i dialog-info
sleep 2
notify-send " " -i notification-device-eject -h string:x-canonical-private-icon-only:allowed
sleep 2
notify-send "WiFi signal found" -i notification-network-wireless-medium
sleep 2
notify-send "WiFi signal lost" -i notification-network-wireless-disconnected
sleep 2
notify-send " " -i notification-audio-volume-medium -h int:value:75 -h string:x-canonical-private-synchronous:volume
sleep 2
notify-send " " -i notification-audio-volume-low -h int:value:30 -h string:x-canonical-private-synchronous:volume
sleep 2
notify-send " " -i notification-display-brightness-high -h int:value:100 -h string:x-canonical-private-synchronous:brightness
sleep 2
notify-send " " -i notification-keyboard-brightness-medium -h int:value:45 -h string:x-canonical-private-synchronous:brightness
sleep 2
notify-send "Testing markup" "Some <b>bold</b>, <u>underlined</u>, <i>italic</i> text. Note, you should not see any marked up text."
sleep 2
notify-send "Jamshed Kakar" "Hey, what about this restaurant? http://www.blafasel.org

Would you go from your place by train or should I pick you up from work? What do you think?"
sleep 2
notify-send "English bubble" "The quick brown fox jumps over the lazy dog." -i network
sleep 2
notify-send "Bubble from Germany" "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark." -i gnome-system
sleep 2
notify-send "Very russian" "Съешь ещё этих мягких французских булок, да выпей чаю." -i dialog-info
sleep 2
notify-send "More from Germany" "Oje, Qualm verwölkt Dix zig Farbtriptychons." -i gnome-globe


