#include "CLWPlatform.h"
#include "CLWDevice.h"
#include "CLWExcept.h"

#include <cassert>
#include <algorithm>
#include <functional>
#include <iterator>

// Create platform based on platform_id passed
// CLWInvalidID is thrown if the id is not a valid OpenCL platform ID
CLWPlatform CLWPlatform::Create(cl_platform_id id, cl_device_type type)
{
    return CLWPlatform(id, type);
}

void CLWPlatform::CreateAllPlatforms(std::vector<CLWPlatform>& platforms)
{
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms;
    status = clGetPlatformIDs(0, nullptr, &numPlatforms);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");

    std::vector<cl_platform_id> platformIds(numPlatforms);
    std::vector<cl_platform_id> validIds;

    status = clGetPlatformIDs(numPlatforms, &platformIds[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");

    cl_device_type type = CL_DEVICE_TYPE_ALL;

    // TODO: this is a workaround for nasty Apple's OpenCL runtime
    // which doesn't allow to have work group sizes > 1 on CPU devices
    // so disable useless CPU
#ifdef __APPLE__
    type = CL_DEVICE_TYPE_GPU;
#endif

    platforms.clear();
    // Only use CL1.2+ platforms
    for (int i = 0; i < (int)platformIds.size(); ++i)
    {
        size_t size = 0;
        status = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, 0, nullptr, &size);

        std::vector<char> version(size);

        status = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, size, &version[0], 0);

        std::string versionstr(version.begin(), version.end());

        if (versionstr.find("1.0") != std::string::npos ||
            versionstr.find("1.1") != std::string::npos)
        {
            continue;
        }

        validIds.push_back(platformIds[i]);
    }

    std::transform(validIds.cbegin(), validIds.cend(), std::back_inserter(platforms),
        std::bind(&CLWPlatform::Create, std::placeholders::_1, type));
}

CLWPlatform::~CLWPlatform()
{
}

void CLWPlatform::GetPlatformInfoParameter(cl_platform_id id, cl_platform_info param, std::string& result)
{
    size_t length = 0;

    cl_int status = clGetPlatformInfo(id, param, 0, nullptr, &length);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformInfo failed");

    std::vector<char> buffer(length);
    status = clGetPlatformInfo(id, param, length, &buffer[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformInfo failed");

    result = &buffer[0];
}

CLWPlatform::CLWPlatform(cl_platform_id id, cl_device_type type)
: ReferenceCounter<cl_platform_id, nullptr, nullptr>(id)
, type_(type)
{
    GetPlatformInfoParameter(*this, CL_PLATFORM_NAME, name_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_PROFILE, profile_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VENDOR, vendor_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VERSION, version_);
}

// Platform info
std::string CLWPlatform::GetName() const
{
    return name_;
}

std::string CLWPlatform::GetProfile() const
{
    return profile_;
}

std::string CLWPlatform::GetVersion() const
{
    return version_;
}

std::string CLWPlatform::GetVendor()  const
{
    return vendor_;
}

std::string CLWPlatform::GetExtensions() const
{
    return extensions_;
}

// Get number of devices
unsigned int                CLWPlatform::GetDeviceCount() const
{
    if (devices_.size() == 0)
    {
        InitDeviceList(type_);
    }

    return (unsigned int)devices_.size();
}

// Get idx-th device
CLWDevice CLWPlatform::GetDevice(unsigned int idx) const
{
    if (devices_.size() == 0)
    {
        InitDeviceList(type_);
    }

    return devices_[idx];
}

void CLWPlatform::InitDeviceList(cl_device_type type) const
{
    cl_uint numDevices = 0;
    cl_int status = clGetDeviceIDs(*this, type, 0, nullptr, &numDevices);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceIDs failed");

    std::vector<cl_device_id> deviceIds(numDevices);
    status = clGetDeviceIDs(*this, type, numDevices, &deviceIds[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceIDs failed");

    for (cl_uint i = 0; i < numDevices; ++i)
    {
        devices_.push_back(CLWDevice(deviceIds[i]));
    }
}
