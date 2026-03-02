#include "stbcamera.h"

#include <csignal>
#include <thread>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        kato::log::cout << KATO_RED << "stbsource_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

int main()
{
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() Starting " STBSOURCE_STR " (" STBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, source_thread;
    long port = 8001;

    // long fullX = 0, fullY = 0;
    // testbed::FrameArea<long> full = {{fullX, fullY}, {fullX + 2840, fullY + 2224}};
    // StbCamera camera("STB Camera", "stb001", port, full);

    testbed::FrameArea<long> roi = {{320 - 100, 240 - 100}, {320 + 100, 240 + 100}};
    StbCamera camera("STB Camera", "stb001", port, roi);

    ZMQLink link(camera.port);
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(camera), std::ref(link));
    source_thread = std::thread(SourceWorker, std::ref(camera));

    listen_thread.join();
    source_thread.join();

    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() Stopping " STBSOURCE_STR " (" STBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    return 0;
}
