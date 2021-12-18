
CXX=g++
CXXFLAGS=--std=c++17 -g -ggdb -O0 -IORB_SLAM2/include/ -IORB_SLAM2/ -Itinyhttp -I/usr/include/eigen3 -Wno-deprecated-declarations -DCOMPILEDWITHC11 -IMiniJson/Source/include/ -Itinyhttp/htcc/
LDFLAGS=-g -ggdb -O0 -L/usr/local/lib -lORB_SLAM2 `pkg-config --libs opencv` -lraspicam -lpthread

OBJS = \
	src/tankserial.o 					\
	src/toojpeg.o 						\
	src/mono_raspi.o 					\
	src/protocol/serializer.o 			\
	src/protocol/vector_stream.o 		\
	src/protocol/packets.o 				\
	src/client_handler.o				\
	build/http.o 						\
	build/websock.o

all: mono_raspi

include tinyhttp/http.mk

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

mono_raspi: $(OBJS)
	$(CXX) $(LDFLAGS) $^ MiniJson/Source/libJson.a -o $@
