#include <print>

#include "icmp/ping.hpp"

namespace asio = boost::asio;

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            std::print("Usage: ping <host>\n");
            return 1;
        }

        asio::io_context ctx;

        std::string localhost = asio::ip::host_name();
        std::string target_host = argv[1];

        icmp::resolver resolver(ctx);
        icmp::endpoint endpoint = *resolver.resolve(icmp::v4(), localhost, "").begin();

        auto info = std::make_shared<icmpInfo>();
        info->hostname = target_host;
        info->localhost = localhost;
        info->local_addr = endpoint.address().to_string();

        endpoint = *resolver.resolve(icmp::v4(), target_host, "").begin();
        info->remote_addr = endpoint.address().to_string();

        icmpChecker ping(ctx, endpoint, info);

        ping.start();
        ctx.run();

        std::print("{}\n", info->to_string());
    }
    catch (const boost::system::system_error &e)
    {
        std::print("{}\n", e.code().message());
    }
    catch(const std::exception& e)
    {
        std::print("{}\n", e.what());
    }
    catch(...)
    {
        std::print("Unknown error\n");
    }

    return 0;
}