#pragma once

#include <string>
#include <cstdint>
#include <ostream>

#include <boost/system/error_code.hpp>

struct checkData
{
    std::string hostname = "";
    uint16_t port = 0;
};


class IChecker
{
public:
    IChecker() = default;
    virtual ~IChecker() = default;

    virtual void async_check(const checkData&) = 0;
    virtual void print(std::ostream &os) const = 0;
    virtual boost::system::error_code has_error() const = 0;
};