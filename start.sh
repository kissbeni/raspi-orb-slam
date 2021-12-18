echo $LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib:/usr/local/lib
./mono_raspi ORB_SLAM2/Vocabulary/ORBvoc.txt Tank.yaml
#gdb --args ./mono_raspi ORB_SLAM2/Vocabulary/ORBvoc.txt Tank.yaml
