
#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <sstream>
#include <limits>
#include <condition_variable>

#include <opencv2/core/core.hpp>

#include <raspicam/raspicam.h>
#include <System.h>
#include <unistd.h>

#include "http.hpp"
#include "tankserial.h"
#include "toojpeg.h"

#define CAPTURE_BUFFER_COUNT 2
#define CAPTURE_WIDTH  (640)
#define CAPTURE_HEIGHT (480)
#define CAPTURE_FORMAT raspicam::RASPICAM_FORMAT_GRAY

class Semaphore {
public:
    Semaphore(int count = 0, int max = 1) : mCount{count}, mMax{max} {}

    inline void notify() noexcept {
        std::unique_lock<std::mutex> lock{mMutex};
        mCount = std::min(mMax, mCount + 1);
        mConditionVar.notify_one();
    }

    inline void wait() noexcept {
        std::unique_lock<std::mutex> lock{mMutex};
        while (!mCount) mConditionVar.wait(lock);
        mCount--;
    }
private:
    std::mutex mMutex;
    std::condition_variable mConditionVar;
    int mCount, mMax;
};

volatile uint8_t lspeed = 0, rspeed = 0;

class TankProtocolHandler : public WebsockClientHandler {
    public:
        void onConnect() override {
            puts("Connect!");
        }

        void onBinaryMessage(const uint8_t* message, const size_t len) override {
            if (len != 4 || message[0] != 0x01)
            {
                puts("Received invalid websock message");
                return;
            }

            printf("left=%d right=%d\n", message[1], message[2]);
            lspeed = message[1];
            rspeed = message[2];
        }

        void onDisconnect() override {
            puts("Disconnect!");
        }
};

struct OverlayPoint {
    float x, y;
    uint8_t flags;
};

HttpServer gHttpServer;
TankSerial gTankSerial("/dev/ttyS0");

raspicam::RaspiCam camera;
size_t cameraBufferSize, captureBufferSize;

uint8_t *cameraBuffers[CAPTURE_BUFFER_COUNT], *tempCameraBuffer;
volatile size_t currentCameraBuffer;

std::mutex           worldPosListMutex;
std::vector<cv::Mat> worldPosList;
std::vector<OverlayPoint> overlayPosList;
float currentFPS;

Semaphore nextFrameSignal(1);
Semaphore frameAvailableSignal(1);

void cameraThread()
{
    while (true)
    {
        nextFrameSignal.wait();

        camera.grab();
        camera.retrieve(tempCameraBuffer, raspicam::RASPICAM_FORMAT_IGNORE);

        memcpy(cameraBuffers[(currentCameraBuffer+1)%2], tempCameraBuffer, cameraBufferSize);
        frameAvailableSignal.notify();
    }
}

void swapCameraBuffers()
{
    currentCameraBuffer = (currentCameraBuffer + 1) % 2;
    nextFrameSignal.notify();
}

const uint8_t* getCameraBuffer()
{
    frameAvailableSignal.wait();
    return cameraBuffers[currentCameraBuffer];
}

std::string createCameraData() {
    std::vector<unsigned char> out;
    out.reserve(cameraBufferSize);
    TooJpeg::writeJpeg([&out](uint8_t x) { out.push_back(x); }, tempCameraBuffer, camera.getWidth(), camera.getHeight(), false, false, 100);

    std::string content;
    content += "--FRAME\r\n";
    content += "Content-Length: " + std::to_string(out.size()) + "\r\n\r\n";
    content.append((char*)out.data(), out.size());
    content += "\r\n";

    return content;
}

class CameraStreamer : public ICanRequestProtocolHandover {
    public:
        void acceptHandover(short& serverSock, short& clientSock) {
            int sig;

            while (serverSock > 0 && clientSock > 0) {
                auto content = createCameraData();

                if ((sig = send(clientSock, content.data(), content.length(), MSG_NOSIGNAL)) <= 0) {
                    printf("Send failed");
                    if (sig == EPIPE) continue;
                    return;
                }

                usleep(1000000/20);
            }
        }
};

int main(int argc, char **argv)
{
    puts("Hello!");

    if (argc != 3)
    {
        std::cerr << "Usage: ./mono_raspi path_to_vocabulary path_to_settings" << std::endl;
        return 1;
    }

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::MONOCULAR, true);

    cout << endl << "-------" << endl;
    std::cout << "Initializing components..." << std::endl;

    int cols = CAPTURE_WIDTH;
    int rows = CAPTURE_HEIGHT;

    std::cout << "Setting up camera (" << cols << "x" << rows << ")" << std::endl;

    cv::Mat im(rows, cols, CV_8UC1);

    std::cout << " OpenCV image matrix: channels=" << im.channels() << ",elemSize=" << im.elemSize() << ",total=" << im.total() << ",storedAt=" << ((void*)im.data) << std::endl;
    memset(im.data, 0, im.total());

    camera.setFormat(CAPTURE_FORMAT);
    camera.setCaptureSize(cols, rows);
    camera.setISO(600);

    if (!camera.open()) {
        puts("Could not open the camera");
        return 1;
    }

    if (!gTankSerial.open())
        return 2;

    puts("Opened serial communication");

    CameraStreamer stream;

    gHttpServer.when("/")->serveFile("index.html");
    gHttpServer.when("/frontlamps/on")->posted([](const HttpRequest& req) {
        for (int i = 4; i < 8; i++)
            gTankSerial.setLedColor(i, 255, 255, 255);
        return HttpResponse{200};
    });
    gHttpServer.when("/frontlamps/off")->posted([](const HttpRequest& req) {
        for (int i = 4; i < 8; i++)
            gTankSerial.clearLed(i);
        return HttpResponse{200};
    });
    gHttpServer.when("/stats")->requested([](const HttpRequest& req) {
        std::stringstream ss;
        ss.precision(6);
        ss << "{\"points\":[";
        bool flag = false;
        {
            std::unique_lock<std::mutex> lock{worldPosListMutex};
            for (auto& x : worldPosList) {
                if (flag) ss << ","; else flag = true;
                ss << "[" << std::fixed << x.at<float>(0) << "," << x.at<float>(1) << "," << x.at<float>(2) << "]";
            }
            ss << "],\"overlay\":[";
            flag = false;
            for (auto& x : overlayPosList) {
                if (flag) ss << ","; else flag = true;
                ss << "{\"x\":" << std::fixed << x.x << ",\"y\":" << x.y << ",\"f\":" << ((int)x.flags) << "}";
            }
        }

        ss << "],\"fps\":" << std::fixed << currentFPS << "}";
        return HttpResponse{200, "application/json", ss.str()};
    });
    gHttpServer.when("/video.mjpeg")->requested([&stream](const HttpRequest& req) {
        HttpResponse res{200};
        res["Age"] = "0";
        res["Cache-Control"] = "no-cache, private";
        res["Pragma"] = "no-cache";
        res["Content-Type"] = "multipart/x-mixed-replace;boundary=FRAME";

        res.setContent(createCameraData());
        res["Content-Length"] = "";

        res.requestProtocolHandover(&stream);
        return res;
    });

    gHttpServer.websocket("/ws")->handleWith<TankProtocolHandler>();

    std::thread t([]() {
        uint8_t last_lspeed = 0, last_rspeed = 0;

        while (true) {
            if (last_lspeed != lspeed || last_rspeed != rspeed) {
                last_lspeed = lspeed;
                last_rspeed = rspeed;

                gTankSerial.speed(lspeed, rspeed);
                continue;
            }

            usleep(100000);
        }
    });

    camera.grab();
    captureBufferSize = camera.getImageTypeSize(CAPTURE_FORMAT);
    cameraBufferSize = captureBufferSize;

    std::cout << "We need " << cameraBufferSize << " bytes to hold a frame" << std::endl;

    for (int i = 0; i < CAPTURE_BUFFER_COUNT; i++) {
        cameraBuffers[i] = new uint8_t[cameraBufferSize];
        std::cout << "    Allocated circular camera buffer part #" << i << " at " << ((void*)cameraBuffers[i]) << std::endl;
    }

    tempCameraBuffer = new uint8_t[captureBufferSize];
    std::cout << "    Allocated temporay frame buffer at " << ((void*)tempCameraBuffer) << " of size " << captureBufferSize << std::endl;

    uint64_t prev_frame = 0;

    std::thread t2{cameraThread};
    std::thread t3{[]() { gHttpServer.startListening(4000); }};

    camera.grab();

    usleep(2000000);
    std::cout << "Starting main loop" << std::endl;

    uint64_t lastWorldSave = 0;

    // Main loop
    while (true)
    {
        uint64_t tframe = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        //std::chrono::steady_clock::time_point grab1 = std::chrono::steady_clock::now();

        /*camera.grab();
        camera.retrieve(im.data, raspicam::RASPICAM_FORMAT_IGNORE);*/
        memcpy(im.data, getCameraBuffer(), cameraBufferSize);
        swapCameraBuffers();

        //double tgrab = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::steady_clock::now() - grab1).count();

        //std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        // Pass the image to the SLAM system
        SLAM.TrackMonocular(im, tframe);

        if ((tframe - lastWorldSave) > 500000) {
            //std::cout << "Updating point list" << std::endl;
            std::unique_lock<std::mutex> lock{worldPosListMutex};
            worldPosList.clear();
            for (auto& p : SLAM.GetTrackedMapPoints()) {
                if (!p) continue;
                //std::cout << ((void*)p) << std::endl;
                worldPosList.push_back(p->GetWorldPos());
            }
            //overlayPosList = SLAM.GetTrackedKeyPointsUn();
            overlayPosList.clear();
            if (SLAM.mpTracker->mLastProcessedState == ORB_SLAM2::Tracking::OK) {
                auto keys = SLAM.mpTracker->mCurrentFrame.mvKeys;
                overlayPosList.resize(keys.size());
                for (size_t i = 0; i < keys.size(); i++) {
                    auto& k = keys[i];
                    auto& p = SLAM.mpTracker->mCurrentFrame.mvpMapPoints[i];

                    auto& overlay = overlayPosList[i];
                    overlay.x = k.pt.x;
                    overlay.y = k.pt.y;
                    overlay.flags = 0;

                    if (!p) continue;

                    overlay.flags |= 1;

                    if (!SLAM.mpTracker->mCurrentFrame.mvbOutlier[i])
                    {
                        overlay.flags |= 2;

                        if (p->Observations() > 0)
                            overlay.flags |= 4;
                    }
                }
            }

            lastWorldSave = tframe;
        }

        //std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        //double ttrack = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();
        //double ttotal = (tframe - prev_frame) / 1000000.0;

        currentFPS = 1.0f/((tframe-prev_frame) / 1000000.0f);
        //printf("grab time: %.04lf, track time: %.04lf, frame time: %llu, total: %.04lf, fps: %.1f\n", tgrab, ttrack, tframe, ttotal, 1.0/((tframe-prev_frame) / 1000000.0));
        prev_frame = tframe;
        // usleep(66666); // 15 fps
        // usleep(40000); // 25 fps
        // usleep(10000);
    }

    // Stop all threads
    SLAM.Shutdown();

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    return 0;
}
