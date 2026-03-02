#ifndef __VMB_FUNCTIONS_H__
#define __VMB_FUNCTIONS_H__

#include <iostream>
#include <algorithm>
#include <VmbCPP/VmbCPP.h>

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
    // err = feature->GetValues()
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
            std::cout << "error = " << error << " : name = " << _name << " : value = " << _value << "\n";
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