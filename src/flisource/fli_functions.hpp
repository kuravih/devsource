#ifndef __FLI_FUNCTIONS_HPP__
#define __FLI_FUNCTIONS_HPP__

#include "libfliusb/libfli.h"

#include <cstring>
#include <stdexcept>

// --------------------------------------------------------------------------------------------------------------------
enum class FliDevice : flidev_t
{
    // NONE = FLIDEVICE_NONE,
    CAMERA = FLIDEVICE_CAMERA,
    FILTERWHEEL = FLIDEVICE_FILTERWHEEL,
    FOCUSER = FLIDEVICE_FOCUSER,
    // HS_FILTERWHEEL = FLIDEVICE_HS_FILTERWHEEL,
    // RAW = FLIDEVICE_RAW,
    // ENUMERATE_BY_CONNECTION = FLIDEVICE_ENUMERATE_BY_CONNECTION,
};
// --------------------------------------------------------------------------------------------------------------------
enum class FliDomain : flidomain_t
{
    // NONE = FLIDOMAIN_NONE,
    PARALLEL_PORT = FLIDOMAIN_PARALLEL_PORT,
    USB = FLIDOMAIN_USB,
    SERIAL = FLIDOMAIN_SERIAL,
    INET = FLIDOMAIN_INET,
    // SERIAL_19200 = FLIDOMAIN_SERIAL_19200,
    // SERIAL_1200 = FLIDOMAIN_SERIAL_1200,
};
// --------------------------------------------------------------------------------------------------------------------
enum class FliFrameType : fliframe_t
{
    NORMAL = FLI_FRAME_TYPE_NORMAL,
    DARK = FLI_FRAME_TYPE_DARK,
    // FLOOD = FLI_FRAME_TYPE_FLOOD,
    // RBI_FLUSH = FLI_FRAME_TYPE_RBI_FLUSH,
};
// --------------------------------------------------------------------------------------------------------------------
enum class FliBinning : long
{
    B_1X = 1,
    B_2X,
    B_3X,
    B_4X,
    B_5X,
    B_6X,
    B_7X,
    B_8X,
    B_9X,
    B_10X,
    B_11X,
    B_12X,
    B_13X,
    B_14X,
    B_15X,
    B_16X = 16
};
// --------------------------------------------------------------------------------------------------------------------
enum class FliFlush : long
{
    F_1X = 1,
    F_2X,
    F_3X,
    F_4X,
    F_5X,
    F_6X,
    F_7X,
    F_8X,
    F_9X,
    F_10X,
    F_11X,
    F_12X,
    F_13X,
    F_14X,
    F_15X,
    F_16X = 16
};
// --------------------------------------------------------------------------------------------------------------------
class FliException : public std::runtime_error
{
private:
    std::string m_message;

public:
    explicit FliException(const LIBFLIAPI _error) : FliException(std::string(strerror((int)-_error))) {}
    explicit FliException(const char *_message) : std::runtime_error(_message)
    {
        m_message = std::string(_message);
    }
    explicit FliException(const std::string _message) : FliException(_message.c_str()) {}
    ~FliException() throw() {}
    const char *what() const throw() override
    {
        return m_message.c_str();
    }
    operator bool() const
    {
        return m_message.empty();
    }
};

#endif //__FLI_FUNCTIONS_HPP__