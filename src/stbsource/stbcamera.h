#ifndef __STBCAMERA_H__
#define __STBCAMERA_H__

#pragma once

#include "testbed/common.hpp"
#include "testbed/shmio_functions.hpp"
#include "kato/truetype.hpp"
#include "kato/log.hpp"
#include "link/zmq_link.hpp"
#include "stbsource_def.h"
#include "stbsource_conf_def.h"
#include "toml11/toml.hpp"

#include <atomic>

volatile std::atomic<bool> busy{true};

// ====================================================================================================================
struct StbCamera
{
    std::string name, serial;
    double px_max;
    double exposureTime_s;
    double temperature_C;
    double gain;
    testbed::FrameArea<long> full, roi;
    shmio::DataType datatype;
    long port;
    shmio::SharedMemory memory;
    shmio::Keyword *shm_exposureTime_s, *shm_temperature_C, *shm_roi_tl_x, *shm_roi_tl_y, *shm_roi_br_x, *shm_roi_br_y, *shm_gain;

    StbCamera(const char *_name, const char *_serial, long _port, const testbed::FrameArea<long> &_roi) : name(_name), serial(_serial), px_max(std::pow(2, 16) - 1), exposureTime_s(0.001), temperature_C(20.0), gain(0.0), full({{0, 0}, {2840, 2224}}), roi(_roi), datatype(shmio::DataType::UINT16), port(_port) {}
    int openStream()
    {
        if (testbed::create_camera_memory(memory, (serial + "_" STBSOURCE_STR).c_str(), full.size(), roi.size(), datatype, serial.c_str(), px_max, port) == 0)
        {
            shm_exposureTime_s = find_keyword("EXPTIME");
            shm_exposureTime_s->value.numf = exposureTime_s;
            shm_temperature_C = find_keyword("TEMP");
            shm_temperature_C->value.numf = temperature_C;
            shm_gain = find_keyword("GAIN");
            shm_gain->value.numf = gain;
            shm_roi_tl_x = find_keyword("ROI.TL.X");
            shm_roi_tl_y = find_keyword("ROI.TL.Y");
            shm_roi_br_x = find_keyword("ROI.BR.X");
            shm_roi_br_y = find_keyword("ROI.BR.Y");
            shm_roi_tl_x->value.numl = roi.tl.x;
            shm_roi_tl_y->value.numl = roi.tl.y;
            shm_roi_br_x->value.numl = roi.br.x;
            shm_roi_br_y->value.numl = roi.br.y;
            return 0;
        }
        return -1;
    }
    int closeStream()
    {
        return shmio::close_shared_memory(memory);
    }
    inline shmio::SharedStorage *get_storage_ptr()
    {
        return shmio::get_storage_ptr(memory);
    }
    inline shmio::Keyword *find_keyword(const char *_name)
    {
        return shmio::find_keyword(memory, _name);
    }
    template <typename Type>
    inline std::span<Type> get_pixels_as()
    {
        return shmio::get_pixels_as<Type>(memory);
    }
    template <typename Type>
    void exposeFrame()
    {
        std::span<Type> pixels = get_pixels_as<Type>();
        std::fill(pixels.begin(), pixels.end(), 0);
        std::this_thread::sleep_for(std::chrono::duration<double>(exposureTime_s));
    }
    template <typename Type>
    void overlay(kato::TrueTypeFont &_ttf, const std::string &_text)
    {
        std::span<Type> pixels = get_pixels_as<Type>();
        _ttf.renderText(pixels.data(), roi.size().width, roi.size().height, 10, 10, _text, px_max, px_max);
    }
    void setExposureTime_s(const double &_exposureTime_s)
    {
        shm_exposureTime_s->value.numf = exposureTime_s = _exposureTime_s;
    }
    void setTemperature_C(const double &_temperature_C)
    {
        shm_temperature_C->value.numf = temperature_C = _temperature_C;
    }
    void setGain(const double &_gain)
    {
        shm_gain->value.numf = gain = _gain;
    }
    void setROI(const testbed::FrameArea<long> &_roi)
    {
        shm_roi_tl_x->value.numl = roi.tl.x = _roi.tl.x;
        shm_roi_tl_y->value.numl = roi.tl.y = _roi.tl.y;
        shm_roi_br_x->value.numl = roi.br.x = _roi.br.x;
        shm_roi_br_y->value.numl = roi.br.y = _roi.br.y;
    }
    ~StbCamera() = default;
};
// ====================================================================================================================
void ListenWorker(StbCamera &_camera, ZMQLink &_link)
{
    kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() Listen thread starting..." << KATO_RESET << std::endl;

    static std::string rxMessage;
    while (_link.isListening.load() && busy.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(LINK_SHORT_SLEEP_US));
        rxMessage = _link.Receive();
        if (rxMessage.size() > 0)
        {
            // kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() rxMessage = " << rxMessage << KATO_RESET << std::endl;

            toml::value data = toml::parse_str(rxMessage);
            std::string sync = "";
            std::ostringstream txStream;
            std::string txMessage;

            try // [settings] exposureTime_s = exposureTime_s_value
            {
                double exposureTime_s = data.at("settings").at("exposureTime_s").as_floating();
                kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() exposureTime_s = " << exposureTime_s << KATO_RESET << std::endl;
                _camera.setExposureTime_s(exposureTime_s);
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"exposureTime_s", _camera.exposureTime_s}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // [settings] temperature_C = temperature_C_value
            {
                double temperature_C = data.at("settings").at("temperature_C").as_floating();
                kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() temperature_C = " << temperature_C << KATO_RESET << std::endl;
                _camera.setTemperature_C(temperature_C);
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"temperature_C", _camera.temperature_C}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // [settings] gain = gain_value
            {
                float gain = data.at("settings").at("gain").as_floating();
                kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() gain = " << gain << KATO_RESET << std::endl;
                _camera.setGain(gain);
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"gain", _camera.gain}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // [settings.nudge] x = amount, y = amount
            {
                int nudge_x = data.at("settings").at("nudge").at("x").as_integer();
                int nudge_y = data.at("settings").at("nudge").at("y").as_integer();
                testbed::FrameArea roi = _camera.roi;
                kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() nudge ROI : " + std::string(roi) + " by (x,y) = (" << nudge_x << "," << nudge_y << ")" << KATO_RESET << std::endl;
                roi.move(nudge_x, nudge_y, _camera.full);
                _camera.setROI(roi);
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"roi", std::string(_camera.roi)}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // Settings = "sync"
            {
                std::string sync = data.at("settings").as_string();
                kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() " << sync << " Received... " << KATO_RESET << std::endl;
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"exposureTime_s", _camera.exposureTime_s}, {"temperature_C", _camera.temperature_C}, {"gain", _camera.gain}, {"roi", std::string(_camera.roi)}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }
        }
    }
    _link.isListening.store(false);

    kato::log::cout << KATO_MAGENTA << "stbcamera.h::ListenWorker() Listen thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================
void SourceWorker(StbCamera &_camera)
{
    kato::log::cout << KATO_MAGENTA << "stbcamera.h::SourceWorker() Source thread starting..." << KATO_RESET << std::endl;
    if (_camera.openStream() == 0)
    {
        kato::TrueTypeFont ttf(KATO_DIR "/ProggyClean.ttf", 12);

        std::chrono::system_clock::time_point t0, t1;
        shmio::SharedStorage *storage = _camera.get_storage_ptr();
        shmio::Keyword *framerate = _camera.find_keyword("FRMRATE");
        std::span<uint16_t> pixels = shmio::get_pixels_as<uint16_t>(_camera.memory);
        _camera.shm_exposureTime_s = _camera.find_keyword("EXPTIME");
        _camera.shm_temperature_C = _camera.find_keyword("TEMP");
        _camera.shm_gain = _camera.find_keyword("GAIN");
        _camera.shm_roi_tl_x = _camera.find_keyword("ROI.TL.X");
        _camera.shm_roi_tl_y = _camera.find_keyword("ROI.TL.Y");
        _camera.shm_roi_br_x = _camera.find_keyword("ROI.BR.X");
        _camera.shm_roi_br_y = _camera.find_keyword("ROI.BR.Y");

        kato::log::cout << KATO_MAGENTA << "  - name : " << _camera.memory.name << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - size : " << _camera.memory.size << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - creationtime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->creationtime)) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - lastaccesstime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->lastaccesstime)) << KATO_RESET << std::endl;

        kato::log::cout << KATO_MAGENTA << "stbcamera.h::SourceWorker() - starting ..." << KATO_RESET << std::endl;
        while (busy.load())
        {
            t0 = std::chrono::system_clock::now();

            // ==== begin critical section ============================================================================
            shmio::wait_for_request(storage);

            // --------------------------------------------------------------------------------------------------------
            _camera.exposeFrame<uint16_t>();
            t1 = std::chrono::system_clock::now();
            framerate->value.numf = kato::function::delta_time_point_to_framerate(t0, t1);
            storage->lastaccesstime = kato::function::time_point_to_timespec(t1);

            _camera.overlay<uint16_t>(ttf, "now     : " + kato::function::TimeStampString(3, "%H:%M:%S", ".", t0) + "\n" +
                                           "FRMRATE : " + std::to_string(framerate->value.numf) + "\n" +
                                           "EXPTIME : " + std::to_string(_camera.shm_exposureTime_s->value.numf) + "\n" +
                                           "TEMP    : " + std::to_string(_camera.shm_temperature_C->value.numf) + "\n" +
                                           "GAIN    : " + std::to_string(_camera.shm_gain->value.numf) + "\n" +
                                           "ROI.TL  : [" + std::to_string(_camera.shm_roi_tl_x->value.numl) + "," + std::to_string(_camera.shm_roi_tl_y->value.numl) + "]" + "\n" +
                                           "ROI.BR  : [" + std::to_string(_camera.shm_roi_br_x->value.numl) + "," + std::to_string(_camera.shm_roi_br_y->value.numl) + "]");

            kato::log::cout << KATO_MAGENTA << "stbcamera.h::SourceWorker() - framerate = " << std::scientific << std::setprecision(5) << framerate->value.numf << KATO_RESET << std::flush;
            // --------------------------------------------------------------------------------------------------------

            shmio::post_response(storage);
            // ==== end critical section ==============================================================================

            std::cout << "\r\33[2K";
        }

        // ==== begin critical section ================================================================================
        // Terminate shared state cleanly
        pthread_mutex_lock(&storage->mutex);
        pthread_cond_broadcast(&storage->has_request_cond);
        pthread_cond_broadcast(&storage->has_response_cond);
        pthread_mutex_unlock(&storage->mutex);
        // ==== end critical section ==================================================================================

        kato::log::cout << KATO_MAGENTA << "stbcamera.h::SourceWorker() - stop ..." << KATO_RESET << std::endl;

        _camera.closeStream();
    }
    kato::log::cout << KATO_MAGENTA << "stbcamera.h::SourceWorker() Source thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================

#endif //__STBCAMERA_H__