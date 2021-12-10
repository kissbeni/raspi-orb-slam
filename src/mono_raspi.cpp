
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
#include "client_handler.h"

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

HttpServer gHttpServer;
TankSerial gTankSerial("/dev/ttyS0");

raspicam::RaspiCam camera;
size_t cameraBufferSize, captureBufferSize;

uint8_t *cameraBuffers[CAPTURE_BUFFER_COUNT], *tempCameraBuffer;
volatile size_t currentCameraBuffer;

std::mutex           slamMutex;
std::vector<cv::Mat> worldPosList;
ReportPacket reportPacket;
MetricsPacket metricsPacket;

Semaphore nextFrameSignal(1);
Semaphore frameAvailableSignal(1);

void cameraThread() {
    while (true) {
        nextFrameSignal.wait();

        camera.grab();
        camera.retrieve(tempCameraBuffer, raspicam::RASPICAM_FORMAT_IGNORE);

        memcpy(cameraBuffers[(currentCameraBuffer+1)%2], tempCameraBuffer, cameraBufferSize);
        frameAvailableSignal.notify();
    }
}

void swapCameraBuffers() {
    currentCameraBuffer = (currentCameraBuffer + 1) % 2;
    nextFrameSignal.notify();
}

const uint8_t* getCameraBuffer() {
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

struct CameraStreamer : public ICanRequestProtocolHandover {
    void acceptHandover(short& serverSock, IClientStream& client, std::unique_ptr<HttpRequest>) {
        while (serverSock > 0 && client.isOpen()) {
            auto content = createCameraData();

            client.send(content.data(), content.length());

            usleep(1000000/20);
        }
    }
};

namespace packetHandlers {
    void handlePacket(MovePacket* pkt) {
        gTankSerial.speed(pkt->mLeftSpeed, pkt->mRightSpeed);
    }

    void handlePacket(StopPacket* pkt) {
        gTankSerial.stop();
    }

    void handlePacket(LedsPacket* pkt) {
        size_t i = 0;
        for (const auto& l : pkt->mLeds)
            gTankSerial.setLedColor(i++, l->mRed, l->mGreen, l->mBlue);
    }
}

// https://rosettacode.org/wiki/Linux_CPU_utilization 
float getCpuUtilization() {
    static size_t previous_idle_time = 0, previous_total_time = 0;
    
    std::ifstream proc_stat("/proc/stat");
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    std::vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));

    if (times.size() < 4)
        return 0;
    
    size_t idle_time = times[3];
    size_t total_time = std::accumulate(times.begin(), times.end(), 0);

    const float idle_time_delta = idle_time - previous_idle_time;
    const float total_time_delta = total_time - previous_total_time;
    return (1.0f - idle_time_delta / total_time_delta) * 100.0f;
}

size_t getTotalSystemMemory() {
    return sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
}

float getMemUtilization() {
    static size_t totalMemory = getTotalSystemMemory() / 1024;
    static constexpr size_t pos = strlen("MemAvailable: ");

    std::ifstream fis{"/proc/meminfo"};
    std::string line;
    while (getline(fis, line)) {
        if (line.find("MemAvailable:", 0) == 0) {
            line = line.substr(pos, line.rfind(" ") - pos);
            size_t avail = std::stoll(line);
            //std::cout << "max:" << totalMemory << ",avail" << avail << std::endl;
            return (1.0f - (((float)avail)/totalMemory)) * 100.0f;
        }
    }

    return 0;
}

vec3 openCvToVec3(const cv::Mat& m) {
    return {m.at<float>(0), m.at<float>(1), m.at<float>(2)};
}

volatile bool gShutdownFlag = false;

int main(int argc, char **argv)
{
    puts("Hello!");

    if (argc != 3)
    {
        std::cerr << "Usage: ./mono_raspi path_to_vocabulary path_to_settings" << std::endl;
        return 1;
    }

    CameraStreamer stream;

    gHttpServer.when("/")->serveFile("index.html");
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
    gHttpServer.when("/SHUTDOWN!")->posted([](const HttpRequest& req) { 
        puts("Got shutdown request");
        gShutdownFlag = true;
        return HttpResponse{200};
    });

    gHttpServer.websocket("/ws")->handleWith<TankProtocolHandler>();

    std::thread t3{[]() { gHttpServer.startListening(4000); }};

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

    std::thread t([]() {
        uint8_t last_lspeed = 0, last_rspeed = 0;

        while (!gShutdownFlag) {
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
    
    camera.grab();

    usleep(2000000);
    std::cout << "Starting main loop" << std::endl;

    uint64_t lastWorldSave = 0;

    size_t prevKeyframeIndex = 1;

    PathPacket pathPacket;

    ORB_SLAM2::System *SLAMptr = &SLAM;

    std::thread networkPushThread([SLAMptr]() {
        while (!gShutdownFlag) {
            //std::cout << "Updating point list" << std::endl;
            reportPacket.mWorldPoints.clear();
            for (auto& p : SLAMptr->GetTrackedMapPoints()) {
                if (!p) continue;
                //std::cout << ((void*)p) << std::endl;
                const auto& pos = p->GetWorldPos();
                reportPacket.mWorldPoints.push_back(openCvToVec3(pos));
            }
            //overlayPosList = SLAMptr->GetTrackedKeyPointsUn();
            reportPacket.mOverlay.clear();
            if (SLAMptr->mpTracker->mLastProcessedState == ORB_SLAM2::Tracking::OK) {
                slamMutex.lock();
                auto keys = SLAMptr->mpTracker->mCurrentFrame.mvKeys;
                slamMutex.unlock();
                reportPacket.mOverlay.resize(keys.size());
                for (size_t i = 0; i < keys.size(); i++) {
                    auto& k = keys[i];
                    auto& p = SLAMptr->mpTracker->mCurrentFrame.mvpMapPoints[i];

                    auto& overlay = reportPacket.mOverlay[i];
                    overlay.mX = k.pt.x;
                    overlay.mY = k.pt.y;
                    overlay.mFlags = 0;

                    if (!p) continue;

                    overlay.mFlags |= 1;

                    if (!SLAMptr->mpTracker->mCurrentFrame.mvbOutlier[i])
                    {
                        overlay.mFlags |= 2;

                        if (p->Observations() > 0)
                            overlay.mFlags |= 4;
                    }
                }
            }

            metricsPacket.mTrackingState = SLAMptr->mpTracker->mLastProcessedState;
            metricsPacket.mCpuUsage = getCpuUtilization();
            metricsPacket.mMemUsage = getMemUtilization();

            TankProtocolHandler::sendToAll(reportPacket);
            TankProtocolHandler::sendToAll(metricsPacket);

            usleep(500000);
        }
    });

    // Main loop
    while (!gShutdownFlag)
    {
        uint64_t tframe = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::chrono::steady_clock::time_point grab1 = std::chrono::steady_clock::now();

        /*camera.grab();
        camera.retrieve(im.data, raspicam::RASPICAM_FORMAT_IGNORE);*/
        memcpy(im.data, getCameraBuffer(), cameraBufferSize);
        swapCameraBuffers();

        double tgrab = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::steady_clock::now() - grab1).count();

        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        // Pass the image to the SLAM system
        {
            std::unique_lock<std::mutex> lock{slamMutex};
            SLAM.TrackMonocular(im, tframe);
        }

        const auto& kfs = SLAM.mpMap->GetAllKeyFrames();
        for (;prevKeyframeIndex < kfs.size(); ++prevKeyframeIndex) {
            const auto kf = kfs[prevKeyframeIndex];
            if (!kf || kf->isBad()) continue;
            pathPacket.mIndex = prevKeyframeIndex;
            pathPacket.mCurrentCameraPos = openCvToVec3(kf->GetCameraCenter());
            TankProtocolHandler::sendToAll(pathPacket);
        }

        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

        double ttrack = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();
        double ttotal = (tframe - prev_frame) / 1000000.0;

        metricsPacket.mFps = 1.0f/((tframe-prev_frame) / 1000000.0f);
        printf("grab time: %.04lf, track time: %.04lf, frame time: %llu, total: %.04lf, fps: %.1f\n", tgrab, ttrack, tframe, ttotal, 1.0/((tframe-prev_frame) / 1000000.0));
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
