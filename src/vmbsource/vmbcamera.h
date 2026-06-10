#ifndef __VMBCAMERA_H__
#define __VMBCAMERA_H__

#pragma once

#include "testbed/common.hpp"
#include "testbed/shmio_functions.hpp"
#include "vmb_functions.hpp"
#include "kato/truetype.hpp"
#include "kato/log.hpp"
#include "link/zmq_link.hpp"
#include "vmbsource_def.h"
#include "vmbsource_conf_def.h"
#include "toml11/toml.hpp"

#include <atomic>

volatile std::atomic<bool> busy{true};
shmio::SharedStorage *g_storage = nullptr;
ZMQLink *g_link = nullptr;

// ====================================================================================================================
struct VmbCamInfo
{
    std::string id;
    std::string serial;
};
// ====================================================================================================================
std::vector<VmbCamInfo> QueryVmbCamInfoList()
{
    VmbCPP::VmbSystem &sys = VmbCPP::VmbSystem::GetInstance(); // Get a reference to the VimbaSystem singleton
    VmbErrorType err;
    std::vector<VmbCamInfo> ret;
    if (err = sys.Startup(); err == VmbErrorSuccess)
    {
        VmbCPP::CameraPtrVector cameras; // A vector of std::shared_ptr<AVT::VmbAPI::Camera> objects
        if (err = sys.GetCameras(cameras); VmbErrorSuccess == err)
        {
            kato::log::cout << KATO_GREEN << "vmbcamera.h::QueryVmbCamInfoList() Cameras found : " << cameras.size() << KATO_RESET << std::endl;

            for (size_t i = 0; i < cameras.size(); i++)
            {
                VmbCamInfo info;
                cameras[i]->GetID(info.id);
                cameras[i]->GetSerialNumber(info.serial);
                ret.push_back(info);
            }
        }
        else
        {
            kato::log::cout << KATO_GREEN << "vmbcamera.h::QueryVmbCamInfoList() Could not list cameras. Error code " << err << KATO_RESET << std::endl;
        }
        sys.Shutdown(); // Close Vimba
    }
    return ret;
}
// ====================================================================================================================
struct VmbCamera
{
    VmbCPP::VmbSystem &vmb;
    VmbCPP::CameraPtr handle;
    std::string name, serial;
    long px_max;
    double exposureTime_s;
    double temperature_C;
    double gain;
    testbed::FrameArea<long> full, roi;
    shmio::DataType datatype;
    long port;
    shmio::SharedMemory memory;
    shmio::Keyword *shm_exposureTime_s, *shm_temperature_C, *shm_roi_tl_x, *shm_roi_tl_y, *shm_roi_br_x, *shm_roi_br_y, *shm_gain;
    std::mutex acquire_mutex;

    VmbCamera(const char *_name, const char *_serial, long _port, const testbed::FrameArea<long> &_roi, double _exposureTime_s = 0.00001, double _temperature_C = 20.0, double _gain = 0.0) : vmb(VmbCPP::VmbSystem::GetInstance()), name(_name), serial(_serial), px_max(std::pow(2, 12) - 1), exposureTime_s(_exposureTime_s), temperature_C(_temperature_C), gain(_gain), roi(_roi), datatype(shmio::DataType::UINT16), port(_port)
    {
        if (VmbErrorType err = vmb.Startup(); err != VmbErrorSuccess)
            throw std::runtime_error("Could not start API, err=" + std::to_string(err));

        if (VmbErrorSuccess != vmb.GetCameraByID(name.c_str(), handle))
        {
            vmb.Shutdown();
        }
        else
        {
            if (VmbErrorSuccess != handle->Open(VmbAccessModeFull))
            {
                vmb.Shutdown();
            }
        }

        std::string name;
        if (VmbErrorSuccess == handle->GetName(name))
            kato::log::cout << KATO_GREEN << "vmbcamera.h::VmbCamera::VmbCamera() Camera opened : " << name << " (" << serial << ")" << KATO_RESET << std::endl;

        // std::string triggerSelector = "FrameStart";
        // VmbSetFeatureByName(handle, "TriggerSelector", triggerSelector);

        // std::string triggerMode = "On";
        // VmbSetFeatureByName(handle, "TriggerMode", triggerMode);

        // std::string triggerSource = "Software";
        // VmbSetFeatureByName(handle, "TriggerSource", triggerSource);

        // std::string triggerActivation = "RisingEdge";
        // VmbSetFeatureByName(handle, "TriggerActivation", triggerActivation);

        // double triggerDelay = 0;
        // VmbSetFeatureByName(handle, "TriggerDelay", triggerDelay);

        std::string pixelFormat = "Mono12"; // or "Mono8";
        VmbSetFeatureByName(handle, "PixelFormat", pixelFormat);

        std::string pixelSize = "Bpp16"; // or "Bpp8";
        VmbSetFeatureByName(handle, "PixelSize", pixelSize);

        std::string exposureAuto = "Off";
        VmbSetFeatureByName(handle, "ExposureAuto", exposureAuto);

        long long exposureTime_us = (long long)(exposureTime_s * 1000000);
        VmbSetFeatureByName(handle, "ExposureTime", exposureTime_us);

        std::string gainAuto = "Off";
        VmbSetFeatureByName(handle, "GainAuto", gainAuto);

        VmbSetFeatureByName(handle, "Gain", gain);

        double gamma = 1.0;
        VmbSetFeatureByName(handle, "Gamma", gamma);

        long long sensorWidth, sensorHeight;
        VmbGetFeatureByName(handle, "SensorWidth", sensorWidth);
        VmbGetFeatureByName(handle, "SensorHeight", sensorHeight);
        full = {{0, 0}, {sensorWidth, sensorHeight}};

        long long offsetX = _roi.tl.x, offsetY = _roi.tl.y, width = _roi.size().width, height = _roi.size().height;
        VmbSetFeatureByName(handle, "Width", width);
        VmbSetFeatureByName(handle, "Height", height);
        VmbSetFeatureByName(handle, "OffsetX", offsetX);
        VmbSetFeatureByName(handle, "OffsetY", offsetY);
        roi = {{offsetX, offsetY}, {offsetX + width, offsetY + height}};
    }
    int openStream()
    {
        std::string shm_name = serial + "_" VMBSOURCE_STR;
        if (testbed::create_camera_memory(memory, shm_name.c_str(), full.size(), roi.size(), datatype, serial.c_str(), px_max, port) != 0)
        {
            shm_unlink(("/" + shm_name + ".shm").c_str());
            if (testbed::create_camera_memory(memory, shm_name.c_str(), full.size(), roi.size(), datatype, serial.c_str(), px_max, port) != 0)
                return -1;
        }
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
            shm_gain = find_keyword("GAIN");
            shm_gain->value.numf = gain;
            return 0;
        }
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
    void exposeFrame()
    {
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
        double exposureTime_us = (double)(exposureTime_s * 1000000);
        VmbSetFeatureByName(handle, "ExposureTime", exposureTime_us);
    }
    void setTemperature_C(double _temperature_C)
    {
        shm_temperature_C->value.numf = temperature_C = _temperature_C;
    }
    void setROI(const testbed::FrameArea<long> &_roi)
    {
        std::lock_guard<std::mutex> lock(acquire_mutex);
        long long offsetX = _roi.tl.x, offsetY = _roi.tl.y, width = _roi.size().width, height = _roi.size().height;
        VmbSetFeatureByName(handle, "Width", width);
        VmbSetFeatureByName(handle, "Height", height);
        VmbSetFeatureByName(handle, "OffsetX", offsetX);
        VmbSetFeatureByName(handle, "OffsetY", offsetY);
        shm_roi_tl_x->value.numl = roi.tl.x = offsetX;
        shm_roi_tl_y->value.numl = roi.tl.y = offsetY;
        shm_roi_br_x->value.numl = roi.br.x = offsetX + width;
        shm_roi_br_y->value.numl = roi.br.y = offsetY + height;
    }
    void setGain(const double &_gain)
    {
        shm_gain->value.numf = gain = _gain;
        VmbSetFeatureByName(handle, "Gain", gain);
    }
    ~VmbCamera()
    {
        handle->Close();
        vmb.Shutdown();
    }
};
// ====================================================================================================================
void ListenWorker(VmbCamera &_camera, ZMQLink &_link)
{
    kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() Listen thread starting..." << KATO_RESET << std::endl;

    static std::string rxMessage;
    while (_link.isListening.load() && busy.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(LINK_SHORT_SLEEP_US));
        rxMessage = _link.Receive();
        if (rxMessage.size() > 0)
        {
            // kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() rxMessage = " << rxMessage << KATO_RESET << std::endl;

            toml::value data = toml::parse_str(rxMessage);
            std::string sync = "";
            std::ostringstream txStream;
            std::string txMessage;

            try // [settings] exposureTime_s = exposureTime_s_value
            {
                double exposureTime_s = data.at("settings").at("exposureTime_s").as_floating();
                kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() exposureTime_s = " << exposureTime_s << KATO_RESET << std::endl;
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

            try // [settings] gain = gain_value
            {
                float gain = data.at("settings").at("gain").as_floating();
                kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() gain = " << gain << KATO_RESET << std::endl;
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
                kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() nudge ROI : " << std::string(roi) << " by (x,y) = (" << nudge_x << "," << nudge_y << ")" << KATO_RESET << std::endl;
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
                kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() syncing..." << KATO_RESET << std::endl;
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"exposureTime_s", _camera.exposureTime_s}, {"gain", _camera.gain}, {"roi", std::string(_camera.roi)}}}}};
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

    kato::log::cout << KATO_MAGENTA << "vmbcamera.h::ListenWorker() Listen thread ending..." << KATO_RESET << std::endl;
}
// ====================================================================================================================
void SourceWorker(VmbCamera &_camera)
{
    kato::log::cout << KATO_MAGENTA << "vmbcamera.h::SourceWorker() Source thread starting..." << KATO_RESET << std::endl;
    if (_camera.openStream() == 0)
    {
        kato::TrueTypeFont ttf(KATO_DIR "/ProggyClean.ttf", 12);

        std::chrono::system_clock::time_point t0, t1;
        shmio::SharedStorage *storage = _camera.get_storage_ptr();
        g_storage = storage;
        shmio::Keyword *framerate = _camera.find_keyword("FRMRATE");
        std::span<uint16_t> pixels = shmio::get_pixels_as<uint16_t>(_camera.memory);
        VmbCPP::FramePtr frame;

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

        kato::log::cout << KATO_MAGENTA << "vmbcamera.h::SourceWorker() - starting ..." << KATO_RESET << std::endl;
        while (busy.load())
        {
            t0 = std::chrono::system_clock::now();

            // ==== begin critical section ============================================================================
            shmio::wait_for_request(storage);

            // --------------------------------------------------------------------------------------------------------
            {
                std::lock_guard<std::mutex> lock(_camera.acquire_mutex);
                if (VmbErrorSuccess == _camera.handle->AcquireSingleImage(frame, 1000))
                {
                    t1 = std::chrono::system_clock::now();
                    framerate->value.numf = kato::function::delta_time_point_to_framerate(t0, t1);

                    VmbUchar_t *pBuffer;
                    frame->GetImage(pBuffer);
                    memcpy(pixels.data(), pBuffer, pixels.size() * shmio::DataTypeSize(_camera.datatype));
                    storage->lastaccesstime = kato::function::time_point_to_timespec(t1);

                    kato::log::cout << KATO_MAGENTA << "vmbcamera.h::SourceWorker() - framerate = " << std::scientific << std::setprecision(5) << framerate->value.numf << KATO_RESET << std::flush;
                }
                else
                {
                    kato::log::cout << KATO_RED << "vmbcamera.h::SourceWorker() AcquireSingleImage() failed" << KATO_RESET << std::endl;
                }
            }
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

        kato::log::cout << KATO_MAGENTA << "vmbcamera.h::SourceWorker() - stop ..." << KATO_RESET << std::endl;

        _camera.closeStream();
    }
    kato::log::cout << KATO_MAGENTA << "vmbcamera.h::SourceWorker() Source thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================

#endif //__VMBCAMERA_H__