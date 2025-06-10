#pragma once

#include <string>
#include <format>

struct icmpInfo
{
    std::string to_string()
    {
        std::string str = std::format("hostname\t: {}\nlocalhost\t: {}\nremote address\t: {}\nlocal address\t: {}\nping succeeded\t: {}\nreply time\t: {}\n",
            hostname, localhost, remote_addr, local_addr, has_success, reply_time);

        return str;
    }

    std::string hostname = "";
    std::string localhost = "";
    std::string remote_addr = "";
    std::string local_addr = "";
    bool has_success = false;
    int64_t reply_time = 0;
};