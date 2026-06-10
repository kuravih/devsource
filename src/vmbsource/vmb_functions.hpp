#ifndef __VMB_FUNCTIONS_H__
#define __VMB_FUNCTIONS_H__

#include <iostream>
#include <algorithm>
#include <VmbCPP/VmbCPP.h>

inline std::string VmbPrintError(const VmbError_t error)
{
    switch (static_cast<VmbErrorType>(error))
    {
    case VmbErrorSuccess:
        return "No error";
    case VmbErrorInternalFault:
        return "Unexpected fault in VmbC or driver";
    case VmbErrorApiNotStarted:
        return "::VmbStartup() was not called before the current command";
    case VmbErrorNotFound:
        return "The designated instance (camera, feature etc.) cannot be found";
    case VmbErrorBadHandle:
        return "The given handle is not valid";
    case VmbErrorDeviceNotOpen:
        return "Device was not opened for usage";
    case VmbErrorInvalidAccess:
        return "Operation is invalid with the current access mode";
    case VmbErrorBadParameter:
        return "One of the parameters is invalid (usually an illegal pointer)";
    case VmbErrorStructSize:
        return "The given struct size is not valid for this version of the API";
    case VmbErrorMoreData:
        return "More data available in a string/list than space is provided";
    case VmbErrorWrongType:
        return "Wrong feature type for this access function";
    case VmbErrorInvalidValue:
        return "The value is not valid; either out of bounds or not an increment of the minimum";
    case VmbErrorTimeout:
        return "Timeout during wait";
    case VmbErrorOther:
        return "Other error";
    case VmbErrorResources:
        return "Resources not available (e.g. memory)";
    case VmbErrorInvalidCall:
        return "Call is invalid in the current context (e.g. callback)";
    case VmbErrorNoTL:
        return "No transport layers are found";
    case VmbErrorNotImplemented:
        return "API feature is not implemented";
    case VmbErrorNotSupported:
        return "API feature is not supported";
    case VmbErrorIncomplete:
        return "The current operation was not completed (e.g. a multiple registers read or write)";
    case VmbErrorIO:
        return "Low level IO error in transport layer";
    case VmbErrorValidValueSetNotPresent:
        return "The valid value set could not be retrieved, since the feature does not provide this property";
    case VmbErrorGenTLUnspecified:
        return "Unspecified GenTL runtime error";
    case VmbErrorUnspecified:
        return "Unspecified runtime error";
    case VmbErrorBusy:
        return "The responsible module/entity is busy executing actions";
    case VmbErrorNoData:
        return "The function has no data to work on";
    case VmbErrorParsingChunkData:
        return "An error occurred parsing a buffer containing chunk data";
    case VmbErrorInUse:
        return "Something is already in use";
    case VmbErrorUnknown:
        return "Error condition unknown";
    case VmbErrorXml:
        return "Error parsing XML";
    case VmbErrorNotAvailable:
        return "Something is not available";
    case VmbErrorNotInitialized:
        return "Something is not initialized";
    case VmbErrorInvalidAddress:
        return "The given address is out of range or invalid for internal reasons";
    case VmbErrorAlready:
        return "Something has already been done";
    case VmbErrorNoChunkData:
        return "A frame expected to contain chunk data does not contain chunk data";
    case VmbErrorUserCallbackException:
        return "A callback provided by the user threw an exception";
    case VmbErrorFeaturesUnavailable:
        return "The XML for the module is currently not loaded; the module could be in the wrong state or the XML could not be retrieved or could not be parsed properly";
    case VmbErrorTLNotFound:
        return "A required transport layer could not be found or loaded";
    case VmbErrorAmbiguous:
        return "An entity cannot be uniquely identified based on the information provided";
    case VmbErrorRetriesExceeded:
        return "Something could not be accomplished with a given number of retries";
    case VmbErrorInsufficientBufferCount:
        return "The operation requires more buffers";
    case VmbErrorCustom:
        return "The minimum error code to use for user defined error codes to avoid conflict with existing error codes";
    default:
        return "Unrecognized VmbErrorType (" + std::to_string(error) + ")";
    }
}

inline void VmbPrintEntries(const VmbCPP::EnumEntry &entry)
{
    std::string name;
    if (VmbErrorSuccess == entry.GetName(name))
        std::cout << name << ",";
}
inline void VmbPrintFeature(const VmbCPP::FeaturePtr &feature)
{
    std::vector<std::string> VmbFeatureStrings = {"VmbFeatureDataUnknown", "VmbFeatureDataInt", "VmbFeatureDataFloat", "VmbFeatureDataEnum", "VmbFeatureDataString", "VmbFeatureDataBool", "VmbFeatureDataCommand", "VmbFeatureDataRaw", "VmbFeatureDataNone"};
    VmbErrorType err;
    std::string display, name, description, category, representation, unit, tooltip;
    VmbFeatureDataType dataType;
    bool isWritable;
    err = feature->GetCategory(category);
    err = feature->GetDataType(dataType);
    err = feature->GetDescription(description);
    err = feature->GetDisplayName(display);
    // feature->GetEntries
    // feature->GetEntry
    // feature->GetFlags
    // feature->GetIncrement
    err = feature->GetName(name);
    // feature->GetPollingTime
    // feature->GetRange
    err = feature->GetRepresentation(representation);
    // feature->GetSelectedFeatures
    // feature->GetSFNCNamespace
    err = feature->GetToolTip(tooltip);
    err = feature->GetUnit(unit);
    // feature->GetValidValueSet
    // feature->GetValue
    // err = feature->GetValues();
    // feature->GetVisibility
    // feature->SetValue
    // feature->HasIncrement
    // feature->IsCommandDone
    // feature->IsReadable
    // feature->IsStreamable
    // feature->IsValueAvailable
    err = feature->IsWritable(isWritable);
    // feature->RegisterObserver
    // feature->RunCommand
    // feature->UnregisterObserver

    std::cout << "Feature" << "\n"
              << "  category       : " << category << "\n"
              << "  display        : " << display << "\n"
              << "  name           : " << name << "\n"
              << "  Writable       : " << (isWritable ? "Yes" : "No") << "\n"
              << "  unit           : " << unit << "\n"
              << "  tooltip        : " << tooltip << "\n"
              << "  representation : " << representation << "\n"
              << "  description    : " << description << "\n"
              << "  dataType       : " << VmbFeatureStrings[dataType] << "\n";

    if (dataType == VmbFeatureDataInt)
    {
        VmbInt64_t imin, imax;
        err = feature->GetRange(imin, imax);
        std::cout << "  range          : [" << imin << " - " << imax << "]\n";
    }
    else if (dataType == VmbFeatureDataFloat)
    {
        double fmin, fmax;
        err = feature->GetRange(fmin, fmax);
        std::cout << "  range          : [" << fmin << " - " << fmax << "]\n";
    }
    else if (dataType == VmbFeatureDataEnum)
    {
        std::cout << "  entries        : [";
        VmbCPP::EnumEntryVector entries;
        if (err = feature->GetEntries(entries); err == VmbErrorSuccess)
            std::for_each(entries.begin(), entries.end(), VmbPrintEntries);
        std::cout << "]\n";
    }
}

inline bool VmbSetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, std::string &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->SetValue(_value.c_str()))
        {
            std::string value;
            if (VmbErrorSuccess == feature->GetValue(value))
            {
                _value = value;
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

inline bool VmbSetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, double &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->SetValue(_value))
        {
            double value;
            if (VmbErrorSuccess == feature->GetValue(value))
            {
                _value = value;
                return true;
            }
            return false;
        }
        return false;
    }
    return false;
}

inline bool VmbSetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, long long &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        VmbError_t error;
        if (error = feature->SetValue(_value); error == VmbErrorSuccess)
        {
            long long value;
            if (VmbErrorSuccess == feature->GetValue(value))
            {
                _value = value;
                return true;
            }
            return false;
        }
        else
        {
            std::cout << "error = " << VmbPrintError(error) << " : name = " << _name << " : value = " << _value << "\n";
        }
        return false;
    }
    return false;
}

inline bool VmbGetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, std::string &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->GetValue(_value))
        {
            return true;
        }
        return false;
    }
    return false;
}

inline bool VmbGetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, double &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->GetValue(_value))
        {
            return true;
        }
        return false;
    }
    return false;
}

inline bool VmbGetFeatureByName(VmbCPP::CameraPtr _camera, const char *_name, long long &_value)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->GetValue(_value))
        {
            return true;
        }
        return false;
    }
    return false;
}

inline bool VmbRunCommandByName(VmbCPP::CameraPtr _camera, const char *_name)
{
    VmbCPP::FeaturePtr feature;
    if (VmbErrorSuccess == _camera->GetFeatureByName(_name, feature))
    {
        if (VmbErrorSuccess == feature->RunCommand())
        {
            bool commandDone = false;
            do
            {
                if (feature->IsCommandDone(commandDone) != VmbErrorSuccess)
                {
                    break;
                }
            } while (commandDone == false);
            return true;
        }
        return false;
    }
    return false;
}

#endif //__VMB_FUNCTIONS_H__