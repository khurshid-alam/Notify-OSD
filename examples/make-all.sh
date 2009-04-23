#!/bin/sh

gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-only.c -o icon-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c sync-icon-only.c -o sync-icon-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-summary-body.c -o icon-summary-body
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-summary.c -o icon-summary
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-value.c -o icon-value
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c summary-body.c -o summary-body
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c summary-only.c -o summary-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c append-hint-example.c -o append-hint-example
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c update-notifications.c -o update-notifications

gmcs -pkg:notify-sharp example-util.cs icon-only.cs -out:icon-only.exe
gmcs -pkg:notify-sharp example-util.cs sync-icon-only.cs -out:sync-icon-only.exe
gmcs -pkg:notify-sharp example-util.cs icon-summary-body.cs -out:icon-summary-body.exe
gmcs -pkg:notify-sharp example-util.cs icon-summary.cs -out:icon-summary.exe
gmcs -pkg:notify-sharp example-util.cs icon-value.cs -out:icon-value.exe
gmcs -pkg:notify-sharp example-util.cs summary-body.cs -out:summary-body.exe
gmcs -pkg:notify-sharp example-util.cs summary-only.cs -out:summary-only.exe
gmcs -pkg:notify-sharp -r:Mono.Posix.dll example-util.cs append-hint-example.cs -out:append-hint-example.exe
gmcs -pkg:notify-sharp -r:Mono.Posix.dll example-util.cs update-notifications.cs -out:update-notifications.exe

