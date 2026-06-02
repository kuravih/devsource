#include "flicamera.h"

#include <csignal>
#include <thread>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        kato::log::cout << KATO_RED << "flisource_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

int main()
{
    kato::log::cout << KATO_GREEN << "flisource_main.cpp::main() Starting " FLISOURCE_STR " (" FLISOURCE_VER_STR ")" << KATO_RESET << std::endl;

    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, source_thread;
    long port = 8003;

    std::vector<FliCamInfo> camInfos = QueryFliCamInfoList();
    if (camInfos.size() == 0)
    {
        kato::log::cout << KATO_RED << "flistream_main.cpp::main() No cameras detected: Stopping " FLISOURCE_STR " (" FLISOURCE_VER_STR ")" << KATO_RESET << std::endl;
        return 0;
    }
    else
    {
        for (FliCamInfo &camInfo : camInfos)
            kato::log::cout << KATO_GREEN << "flistream_main.cpp::main() Camera : [dev : " << camInfo.dev << ", model : " << camInfo.model << ", serial : " << camInfo.serial << "]" << KATO_RESET << std::endl;
    }

    // long fullX = 0, fullY = 0;
    // testbed::FrameArea<long> full = {{fullX, fullY}, {fullX + 2840, fullY + 2224}};
    // FliCamera camera(camInfos[0].dev, camInfos[0].model, camInfos[0].serial, port, full);

    long offset_x = 1433, offset_y = 1140; // get value from the cursor
    testbed::FrameArea<long> roi = {{offset_x - 128, offset_y - 128}, {offset_x + 128, offset_y + 128}};
    FliCamera camera(camInfos[0].dev, camInfos[0].model, camInfos[0].serial, port, roi);

    ZMQLink link(camera.port);
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(camera), std::ref(link));
    source_thread = std::thread(SourceWorker, std::ref(camera));

    listen_thread.join();
    source_thread.join();

    kato::log::cout << KATO_GREEN << "flisource_main.cpp::main() Stopping " FLISOURCE_STR " (" FLISOURCE_VER_STR ")" << KATO_RESET << std::endl;

    return 0;
}
