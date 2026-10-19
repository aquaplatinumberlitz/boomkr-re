#!/bin/bash
OUT=/tmp/meas10; mkdir -p $OUT
cd /home/admin/boom-re
rm -f /tmp/.X82-lock
Xvfb :82 -screen 0 1024x768x24 -nolisten tcp >/dev/null 2>&1 &
sleep 2; export DISPLAY=:82
cd wine/Boom; rm -f CALog.log
env -u WAYLAND_DISPLAY WINEDEBUG=-all WINEPREFIX=$HOME/.wine-vn timeout 260 wine ca_fixed_patched.exe > $OUT/client.txt 2>&1 &
sleep 12
for a in $(seq 1 12); do
  grep -q 'dne:10' CALog.log 2>/dev/null && break
  xdotool mousemove 575 602 click 1; sleep 0.4; xdotool type --delay 50 tester; sleep 0.2
  xdotool mousemove 575 634 click 1; sleep 0.4; xdotool type --delay 50 1234; sleep 0.2
  xdotool mousemove 581 733 click 1; sleep 8
done
sleep 10; xdotool mousemove 650 393 click 1           # vào phòng
for a in $(seq 1 20); do grep -q 'dne:11' CALog.log 2>/dev/null && break; sleep 4; done
sleep 8
xdotool mousemove 760 672 click 1                      # SẴN SÀNG
sleep 12
import -window root $OUT/room.png 2>/dev/null
for a in $(seq 1 30); do grep -q 'dne:13' CALog.log 2>/dev/null && break; sleep 3; done
sleep 10
import -window root $OUT/pre.png 2>/dev/null
xdotool keydown Right
i=0; while [ $i -lt 14 ]; do i=$((i+1)); import -window root $OUT/mv_$i.png 2>/dev/null; sleep 0.15; done
xdotool keyup Right; sleep 1
import -window root $OUT/post.png 2>/dev/null
echo "shots=$(ls $OUT/*.png | wc -l)" > $OUT/state.txt
