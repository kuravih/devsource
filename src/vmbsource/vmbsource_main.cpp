#include "vmbcamera.h"

#include <csignal>
#include <thread>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        kato::log::cout << KATO_RED << "vmbsource_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

int main()
{
    kato::log::cout << KATO_GREEN << "vmbsource_main.cpp::main() Starting " VMBSOURCE_STR " (" VMBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, source_thread;
    long port = 8002;

    std::vector<VmbCamInfo> camInfos = QueryVmbCamInfoList();
    if (camInfos.size() == 0)
    {
        kato::log::cout << KATO_RED << "vmbsource_main.cpp::" << "main() " << "No cameras detected: Stopping " VMBSOURCE_STR " (" VMBSOURCE_VER_STR ")" << KATO_RESET << std::endl;
        return 1;
    }
    else
    {
        for (VmbCamInfo &camInfo : camInfos)
            kato::log::cout << KATO_GREEN << "vmbsource_main.cpp::" << "main() " << "Camera : [id : " << camInfo.id << ", serial : " << camInfo.serial << "]" << KATO_RESET << std::endl;
    }

    long OffsetX = 656, OffsetY = 32;
    testbed::FrameArea<long> roi = {{OffsetX, OffsetY}, {OffsetX + 512, OffsetY + 512}};
    VmbCamera camera(camInfos[0].id.c_str(), camInfos[0].serial.c_str(), port, roi);

    ZMQLink link(camera.port);
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(camera), std::ref(link));
    source_thread = std::thread(SourceWorker, std::ref(camera));

    listen_thread.join();
    source_thread.join();

    kato::log::cout << KATO_GREEN << "vmbsource_main.cpp::main() Stopping " VMBSOURCE_STR " (" VMBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    return 0;
}
