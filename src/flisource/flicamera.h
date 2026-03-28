#ifndef __FLICAMERA_H__
#define __FLICAMERA_H__

#pragma once

#include "testbed/common.hpp"
#include "testbed/shmio_functions.hpp"
#include "fli_functions.hpp"
#include "kato/truetype.hpp"
#include "kato/log.hpp"
#include "link/zmq_link.hpp"
#include "flisource_def.h"
#include "flisource_conf_def.h"
#include "toml11/toml.hpp"

#include <atomic>

volatile std::atomic<bool> busy{true};

// ====================================================================================================================
struct FliCamInfo
{
    char dev[128], model[128], serial[128];
};
// ====================================================================================================================
std::vector<FliCamInfo> QueryFliCamInfoList()
{
    FliCamInfo info;
    std::vector<FliCamInfo> ret;

    char **list = NULL;
    flidomain_t domain = static_cast<long>(FliDomain::USB) | static_cast<long>(FliDevice::CAMERA);

    LIBFLIAPI error = FLIList(domain, &list);
    if (error < 0)
    {
        kato::log::cerr << KATO_RED << "flicamera.cpp::QueryUSBFliCamInfoList() FLIList failed with error code " << error << KATO_RESET << std::endl;
        if (list)
        {
            FLIFreeList(list);
        }
        return ret;
    }
    else
    {
        int count = 0;
        while (list[count])
        {
            kato::log::cout << KATO_GREEN << "flicamera.cpp::QueryUSBFliCamInfoList() Found device: " << list[count] << KATO_RESET << std::endl;
            std::vector<std::string> dev_model = kato::function::split(list[count], ';');

            std::strncpy(info.dev, dev_model[0].c_str(), sizeof(info.dev) - 1);
            info.dev[sizeof(info.dev) - 1] = '\0';

            flidev_t handle;
            error = FLIOpen(&handle, info.dev, domain);
            if (error < 0)
            {
                kato::log::cerr << KATO_RED << "flicamera.cpp::QueryUSBFliCamInfoList() FLIOpen() failed with error code " << error << KATO_RESET << std::endl;
                if (list)
                {
                    FLIFreeList(list);
                }
                return ret;
            }
            else
            {
                error = FLIGetSerialString(handle, info.serial, sizeof(info.serial));
                if (error < 0)
                {
                    kato::log::cerr << KATO_RED << "flicamera.cpp::QueryUSBFliCamInfoList() FLIGetSerialString() failed with error code " << error << KATO_RESET << std::endl;
                    FLIClose(handle);
                    if (list)
                    {
                        FLIFreeList(list);
                    }
                    return ret;
                }
                error = FLIGetModel(handle, info.model, sizeof(info.model));
                if (error < 0)
                {
                    kato::log::cerr << KATO_RED << "flicamera.cpp::QueryUSBFliCamInfoList() FLIGetModel() failed with error code " << error << KATO_RESET << std::endl;
                    FLIClose(handle);
                    if (list)
                    {
                        FLIFreeList(list);
                    }
                    return ret;
                }
            }
            FLIClose(handle);
            ret.push_back(info);
            count++;
        }
    }
    return ret;
}
// ====================================================================================================================
struct FliCamera
{
    flidev_t handle;
    char dev[16], model[16], serial[16];
    double px_max;
    double exposureTime_s;
    double temperature_C;
    long hwrev, fwrev;
    double pxw, pxh;
    testbed::FrameArea<long> full, roi, visible;
    shmio::DataType datatype;
    long port;
    shmio::SharedMemory memory;
    shmio::Keyword *shm_exposureTime_s, *shm_temperature_C, *shm_roi_tl_x, *shm_roi_tl_y, *shm_roi_br_x, *shm_roi_br_y, *shm_gain;

    FliCamera(const char *_dev, const char *_model, const char *_serial, long _port, const testbed::FrameArea<long> &_roi) : px_max(std::pow(2, 12) - 1), exposureTime_s(0.001), temperature_C(20.0), roi(_roi), datatype(shmio::DataType::UINT16), port(_port)
    {
        strncpy(dev, _dev, sizeof(dev) - 1);
        strncpy(model, _model, sizeof(model) - 1);
        strncpy(serial, _serial, sizeof(serial) - 1);
        flidomain_t domain = static_cast<long>(FliDomain::USB) | static_cast<long>(FliDevice::CAMERA);
        if (LIBFLIAPI error = FLIOpen(&handle, dev, domain))
            throw FliException(error);
        if (LIBFLIAPI error = FLIGetHWRevision(handle, &hwrev))
            throw FliException(error);
        if (LIBFLIAPI error = FLIGetFWRevision(handle, &fwrev))
            throw FliException(error);
        if (LIBFLIAPI error = FLIGetPixelSize(handle, &pxw, &pxh))
            throw FliException(error);
        if (LIBFLIAPI error = FLIGetArrayArea(handle, &full.tl.x, &full.tl.y, &full.br.x, &full.br.y))
            throw FliException(error);
        if (LIBFLIAPI error = FLIGetVisibleArea(handle, &visible.tl.x, &visible.tl.y, &visible.br.x, &visible.br.y))
            throw FliException(error);
        if (LIBFLIAPI error = FLISetImageArea(handle, roi.tl.x, roi.tl.y, roi.br.x, roi.br.y))
            throw FliException(error);
        long exposureTime_ms = (long)(exposureTime_s * 1000);
        if (LIBFLIAPI error = FLISetExposureTime(handle, exposureTime_ms))
            throw FliException(error);
        if (LIBFLIAPI error = FLISetTemperature(handle, temperature_C))
            throw FliException(error);
        setFrameType(FliFrameType::NORMAL);
        setVBinning(FliBinning::B_1X);
        setHBinning(FliBinning::B_1X);
        if (LIBFLIAPI error = FLIControlBackgroundFlush(handle, FLI_BGFLUSH_STOP))
            throw FliException(error);
        setNFlushes(FliFlush::F_1X);
        if (LIBFLIAPI error = FLISetCameraMode(handle, 0))
            throw FliException(error);
        kato::log::cout << KATO_MAGENTA << "flicamera.h::FliCamera() full = " << std::string(full) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "flicamera.h::FliCamera() visible = " << std::string(visible) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "flicamera.h::FliCamera() roi = " << std::string(roi) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "flicamera.h::FliCamera() exposureTime_s = " << exposureTime_s << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "flicamera.h::FliCamera() temperature_C = " << temperature_C << KATO_RESET << std::endl;
    }
    int openStream()
    {
        if (testbed::create_camera_memory(memory, (std::string(serial) + "_" FLISOURCE_STR).c_str(), full.size(), roi.size(), datatype, serial, px_max, port) == 0)
        {
            shm_exposureTime_s = find_keyword("EXPTIME");
            shm_exposureTime_s->value.numf = exposureTime_s;
            shm_temperature_C = find_keyword("TEMP");
            shm_temperature_C->value.numf = temperature_C;
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
    void overlay(kato::TrueTypeFont &_ttf, const std::string &_text)
    {
        std::span<Type> pixels = get_pixels_as<Type>();
        _ttf.renderText(pixels.data(), roi.size().width, roi.size().height, 10, 10, _text, px_max, px_max);
    }
    void setExposureTime_s(const double &_exposureTime_s)
    {
        if (LIBFLIAPI error = FLICancelExposure(handle))
            throw FliException(error);
        shm_exposureTime_s->value.numf = exposureTime_s = _exposureTime_s;
        long exposureTime_ms = (long)(exposureTime_s * 1000);
        if (LIBFLIAPI error = FLISetExposureTime(handle, exposureTime_ms))
            throw FliException(error);
    }
    void setTemperature_C(const double &_temperature_C)
    {
        shm_temperature_C->value.numf = temperature_C = _temperature_C;
        if (LIBFLIAPI error = FLISetTemperature(handle, (double)_temperature_C))
            throw FliException(error);
    }
    void updateTemperature_C()
    {
        if (LIBFLIAPI error = FLIGetTemperature(handle, &temperature_C))
            throw FliException(error);
        shm_temperature_C->value.numf = temperature_C;
    }
    void setFrameType(const FliFrameType &_type)
    {
        if (LIBFLIAPI error = FLISetFrameType(handle, (fliframe_t)_type))
            throw FliException(error);
    }
    void setVBinning(const FliBinning &_binning)
    {
        if (LIBFLIAPI error = FLISetVBin(handle, (long)_binning))
            throw FliException(error);
    }
    void setHBinning(const FliBinning &_binning)
    {
        if (LIBFLIAPI error = FLISetHBin(handle, (long)_binning))
            throw FliException(error);
    }
    void setNFlushes(const FliFlush &_flush)
    {
        if (LIBFLIAPI error = FLISetNFlushes(handle, (long)_flush))
            throw FliException(error);
    }
    void setROI(const testbed::FrameArea<long> &_roi)
    {
        long offsetX = _roi.tl.x, offsetY = _roi.tl.y, width = _roi.size().width, height = _roi.size().height;
        if (LIBFLIAPI error = FLISetImageArea(handle, _roi.tl.x, _roi.tl.y, _roi.br.x, _roi.br.y))
            throw FliException(error);
        shm_roi_tl_x->value.numl = roi.tl.x = offsetX;
        shm_roi_tl_y->value.numl = roi.tl.y = offsetY;
        shm_roi_br_x->value.numl = roi.br.x = offsetX + width;
        shm_roi_br_y->value.numl = roi.br.y = offsetY + height;
    }
    testbed::FrameArea<long> getROI() const
    {
        return roi;
    }
    long getRemainingExpTime_ms()
    {
        long remExpTime_ms = 0;
        if (LIBFLIAPI error = FLIGetExposureStatus(handle, &remExpTime_ms))
            throw FliException(error);
        else
            return remExpTime_ms;
    }
    void exposeFrame()
    {
        if (LIBFLIAPI error = FLIExposeFrame(handle))
            throw FliException(error);
        long remExpTime_ms = (long)(exposureTime_s * 1000);
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // NOTE: This sleep is necessary because FLIGetExposureStatus will return 0 seconds left if called too quickly after starting the exposure
            remExpTime_ms = getRemainingExpTime_ms();
            if (remExpTime_ms == 0)
                break;
            if (remExpTime_ms > 1000)
                std::this_thread::sleep_for(std::chrono::seconds(1));
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(remExpTime_ms));
        }
        if (LIBFLIAPI error = FLIEndExposure(handle))
            throw FliException(error);
    }
    void grabFrame(std::span<uint16_t> &_pixels)
    {
        size_t nread = 0;
        while (FLIGrabFrame(handle, _pixels.data(), memory.size, &nread) < 0)
        {
            kato::log::cout << KATO_RED << "grabFrame() Error. Retrying..." << KATO_RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    ~FliCamera()
    {
        FLICancelExposure(handle);
        FLIControlBackgroundFlush(handle, FLI_BGFLUSH_STOP);
        FLIClose(handle);
    }
};
// ====================================================================================================================
void ListenWorker(FliCamera &_camera, ZMQLink &_link)
{
    kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() Listen thread starting..." << KATO_RESET << std::endl;

    static std::string rxMessage;
    while (_link.isListening.load() && busy.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(LINK_SHORT_SLEEP_US));
        rxMessage = _link.Receive();
        if (rxMessage.size() > 0)
        {
            // kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() rxMessage = " << rxMessage << KATO_RESET << std::endl;

            toml::value data = toml::parse_str(rxMessage);
            std::string sync = "";
            std::ostringstream txStream;
            std::string txMessage;

            try // [settings] exposureTime_s = exposureTime_s_value
            {
                double exposureTime_s = data.at("settings").at("exposureTime_s").as_floating();
                kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() exposureTime_s = " << exposureTime_s << KATO_RESET << std::endl;
                _camera.setExposureTime_s(exposureTime_s);
                exposureTime_s = _camera.exposureTime_s;
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"exposureTime_s", exposureTime_s}}}}};
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
                kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() temperature_C = " << temperature_C << KATO_RESET << std::endl;
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

            try // [settings.nudge] x = amount, y = amount
            {
                int nudge_x = data.at("settings").at("nudge").at("x").as_integer();
                int nudge_y = data.at("settings").at("nudge").at("y").as_integer();
                testbed::FrameArea roi = _camera.roi;
                kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() nudge ROI : " << std::string(roi) << " by (x,y) = (" << nudge_x << "," << nudge_y << ")" << KATO_RESET << std::endl;
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
                kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() " << sync << " Received... " << KATO_RESET << std::endl;
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"exposureTime_s", _camera.exposureTime_s}, {"temperature_C", _camera.temperature_C}, {"roi", std::string(_camera.roi)}}}}};
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

    kato::log::cout << KATO_MAGENTA << "flicamera.h::ListenWorker() Listen thread ending..." << KATO_RESET << std::endl;
}
// ====================================================================================================================
void SourceWorker(FliCamera &_camera)
{
    kato::log::cout << KATO_MAGENTA << "flicamera.h::SourceWorker() Source thread starting..." << KATO_RESET << std::endl;
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

        kato::log::cout << KATO_MAGENTA << "flicamera.h::SourceWorker() - starting ..." << KATO_RESET << std::endl;
        while (busy.load())
        {
            t0 = std::chrono::system_clock::now();

            // ==== begin critical section ============================================================================
            shmio::wait_for_request(storage);

            // --------------------------------------------------------------------------------------------------------
            _camera.exposeFrame();
            _camera.grabFrame(pixels);
            t1 = std::chrono::system_clock::now();
            framerate->value.numf = kato::function::delta_time_point_to_framerate(t0, t1);
            // _camera.overlay<uint16_t>(ttf, "now     : " + kato::function::TimeStampString(3, "%H:%M:%S", ".", t0) + "\n" +
            //                                    "FRMRATE : " + std::to_string(framerate->value.numf) + "\n" +
            //                                    "EXPTIME : " + std::to_string(_camera.shm_exposureTime_s->value.numl) + "\n" +
            //                                    "TEMP    : " + std::to_string(_camera.shm_temperature_C->value.numf) + "\n" +
            //                                    "GAIN    : " + std::to_string(_camera.shm_gain->value.numf) + "\n" +
            //                                    "ROI.TL  : [" + std::to_string(_camera.shm_roi_tl_x->value.numl) + "," + std::to_string(_camera.shm_roi_tl_y->value.numl) + "]" + "\n" +
            //                                    "ROI.BR  : [" + std::to_string(_camera.shm_roi_br_x->value.numl) + "," + std::to_string(_camera.shm_roi_br_y->value.numl) + "]");

            storage->lastaccesstime = kato::function::time_point_to_timespec(t1);

            kato::log::cout << KATO_MAGENTA << "flicamera.h::SourceWorker() - framerate = " << std::scientific << std::setprecision(5) << framerate->value.numf << KATO_RESET << std::flush;
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

        kato::log::cout << KATO_MAGENTA << "flicamera.h::SourceWorker() - stop ..." << KATO_RESET << std::endl;

        _camera.closeStream();
    }
    kato::log::cout << KATO_MAGENTA << "flicamera.h::SourceWorker() Source thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================

#endif //__FLICAMERA_H__