
CXX=g++
CXXFLAGS=--std=c++17 -g -ggdb -IORB_SLAM2/include/ -IORB_SLAM2/ -Itinyhttp -I/usr/include/eigen3 -Ofast -Wno-deprecated-declarations -DCOMPILEDWITHC11 -IMiniJson/Source/include/ -Itinyhttp/htcc/
LDFLAGS=-L/usr/local/lib -lORB_SLAM2 `pkg-config --libs opencv` -lraspicam -lpthread

OBJS = tankserial.o toojpeg.o mono_raspi.o build/http.o build/websock.o

all: mono_raspi

include tinyhttp/http.mk

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

mono_raspi: $(OBJS)
	$(CXX) $(LDFLAGS) $^ MiniJson/Source/libJson.a -o $@
