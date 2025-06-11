#include <print>

#include "icmp/icmp_cheker.hpp"
#include "tcp/port_checker.hpp"

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

        // std::string localhost = asio::ip::host_name();
        // std::string target_host = argv[1];

        // icmp::resolver resolver(ctx);
        // icmp::endpoint endpoint = *resolver.resolve(icmp::v4(), localhost, "").begin();

        // auto info = std::make_shared<icmpInfo>();
        // info->hostname = target_host;
        // info->localhost = localhost;
        // info->local_addr = endpoint.address().to_string();

        // endpoint = *resolver.resolve(icmp::v4(), target_host, "").begin();
        // info->remote_addr = endpoint.address().to_string();

        // icmpChecker ping(ctx, endpoint, info);

        // ping.start();

        tcpPortChecker port_checker(ctx);
        port_checker.async_check(argv[1], 80);

        ctx.run();

        tcpInfo result = port_checker.get_result();

        if (result.ec.value() != 0)
        {
            throw boost::system::system_error(result.ec);
        }
        
        std::print("{:<18} : {}\n", "hostname", result.hostname);
        std::print("{:<18} : {}\n", "remote address", result.remote_addr);
        std::print("{:<18} : {}\n", "port", result.port);
        std::print("{:<18} : {}\n", "tcp test succeeded", result.has_success);

        // std::print("{}\n", info->to_string());
    }
    catch (const boost::system::system_error &e)
    {
        std::print("{}\n", e.code().message());
    }
    catch (const std::exception &e)
    {
        std::print("{}\n", e.what());
    }
    catch (...)
    {
        std::print("Unknown error\n");
    }

    return 0;
}