echo $LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib
sudo perf record -F 999 -a -g -- ./mono_raspi ORB_SLAM2/Vocabulary/ORBvoc.txt Tank.yaml
sudo perf script | ../FlameGraph/stackcollapse-perf.pl --all | ../FlameGraph/flamegraph.pl > /tmp/test.svg
#gdb --args ./mono_raspi ORB_SLAM2/Vocabulary/ORBvoc.txt Tank.yaml
