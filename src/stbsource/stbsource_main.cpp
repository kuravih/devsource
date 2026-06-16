#include "stbcamera.h"

#include <csignal>
#include <thread>
#include <getopt.h>
#include <pthread.h>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        if (g_storage)
        {
            g_storage->has_request = true;
            pthread_cond_broadcast(&g_storage->has_request_cond);
        }
        if (g_link)
            g_link->isListening.store(false);
        kato::log::cout << KATO_RED << "stbsource_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

int main(int argc, char *argv[])
{
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() Starting " STBSOURCE_STR " (" STBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, source_thread;

    // defaults
    long port = 8001;
    double exposureTime_s = 0.001;
    double temperature_C = 20.0;
    double gain = 0.0;
    std::string serial = "stb001";
    testbed::FrameArea<long> roi = {{220, 140}, {420, 340}};
    // long fullX = 0, fullY = 0;
    // testbed::FrameArea<long> full = {{fullX, fullY}, {fullX + 2840, fullY + 2224}};

    // CLI args
    bool set_port = false, set_roi = false, set_exposure = false, set_temperature = false, set_gain = false;

    static struct option long_options[] = {
        {"port", required_argument, nullptr, 'p'},
        {"roi", required_argument, nullptr, 'r'},
        {"exposure", required_argument, nullptr, 'e'},
        {"temperature", required_argument, nullptr, 't'},
        {"gain", required_argument, nullptr, 'g'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "p:r:e:t:g:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = std::stol(optarg);
            set_port = true;
            break;
        case 'e':
            exposureTime_s = std::stod(optarg);
            set_exposure = true;
            break;
        case 't':
            temperature_C = std::stod(optarg);
            set_temperature = true;
            break;
        case 'g':
            gain = std::stod(optarg);
            set_gain = true;
            break;
        case 'r':
        {
            long x0, y0, x1, y1;
            if (std::sscanf(optarg, "%ld,%ld,%ld,%ld", &x0, &y0, &x1, &y1) == 4)
            {
                roi = {{x0, y0}, {x1, y1}};
                set_roi = true;
            }
            break;
        }
        default:
            break;
        }
    }

    // shared memory fallback for unset parameters
    if (!set_port || !set_roi || !set_exposure || !set_temperature || !set_gain)
    {
        shmio::SharedMemory prev;
        if (shmio::open_shared_memory(prev, (serial + "_" STBSOURCE_STR).c_str()) == 0)
        {
            shmio::Keyword *kw;
            if (!set_port && (kw = shmio::find_keyword(prev, "PORT")))
                port = kw->value.numl;
            if (!set_exposure && (kw = shmio::find_keyword(prev, "EXPTIME")))
                exposureTime_s = kw->value.numf;
            if (!set_temperature && (kw = shmio::find_keyword(prev, "TEMP")))
                temperature_C = kw->value.numf;
            if (!set_gain && (kw = shmio::find_keyword(prev, "GAIN")))
                gain = kw->value.numf;
            if (!set_roi)
            {
                shmio::Keyword *tlx = shmio::find_keyword(prev, "ROI.TL.X");
                shmio::Keyword *tly = shmio::find_keyword(prev, "ROI.TL.Y");
                shmio::Keyword *brx = shmio::find_keyword(prev, "ROI.BR.X");
                shmio::Keyword *bry = shmio::find_keyword(prev, "ROI.BR.Y");
                if (tlx && tly && brx && bry)
                    roi = {{tlx->value.numl, tly->value.numl}, {brx->value.numl, bry->value.numl}};
            }
            shmio::close_shared_memory(prev);
        }
    }

    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() port = " << port << KATO_RESET << std::endl;
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() exposure time = " << exposureTime_s << " s" << KATO_RESET << std::endl;
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() temperature = " << temperature_C << " C" << KATO_RESET << std::endl;
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() gain = " << gain << KATO_RESET << std::endl;
    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() roi = ((" << roi.br.x << "," << roi.br.y << "),(" << roi.tl.x << "," << roi.tl.y << "))" << KATO_RESET << std::endl;

    StbCamera camera("STB Camera", serial.c_str(), port, roi);
    camera.exposureTime_s = exposureTime_s;
    camera.temperature_C = temperature_C;
    camera.gain = gain;

    ZMQLink link(camera.port);
    g_link = &link;
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(camera), std::ref(link));
    source_thread = std::thread(SourceWorker, std::ref(camera));

    listen_thread.join();
    source_thread.join();

    kato::log::cout << KATO_GREEN << "stbsource_main.cpp::main() Stopping " STBSOURCE_STR " (" STBSOURCE_VER_STR ")" << KATO_RESET << std::endl;

    return 0;
}
