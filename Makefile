
CXX=/usr/local/clang12/bin/clang++
CXXFLAGS=--std=c++17 -g -ggdb -IORB_SLAM2/include/ -IORB_SLAM2/ -Itinyhttp -I/usr/include/eigen3 -Ofast -Wno-deprecated-declarations -DCOMPILEDWITHC11
LDFLAGS=-LORB_SLAM2/lib/ -L/usr/local/lib -L/usr/local/clang12/lib -lcrypto -lORB_SLAM2 -lpangolin `pkg-config --libs opencv` -lraspicam -lpthread

OBJS = tankserial.o tinyhttp/http.o toojpeg.o mono_raspi.o

all: mono_raspi

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

mono_raspi: $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@
