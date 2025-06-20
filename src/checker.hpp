#pragma once

#include <boost/system/error_code.hpp>
#include <cstdint>
#include <ostream>
#include <string>

struct checkData {
	checkData() = default;
	checkData(const std::string& h, uint16_t p) : hostname(h), port(p) {}
	~checkData() = default;

	std::string hostname = "";
	uint16_t port = 0;
};

class IChecker
{
    public:
	IChecker() = default;
	virtual ~IChecker() = default;

	virtual void async_check(const checkData&) = 0;
	virtual void print(std::ostream& os) const = 0;
	virtual boost::system::error_code has_error() const = 0;
};