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

        // tcpPortChecker port_checker(ctx);
        // port_checker.async_check(argv[1], 80);

        // ctx.run();

        // tcpInfo result = port_checker.get_result();

        // if (result.ec.value() != 0)
        // {
        //     throw boost::system::system_error(result.ec);
        // }
        
        // std::print("{:<18} : {}\n", "hostname", result.hostname);
        // std::print("{:<18} : {}\n", "remote address", result.remote_addr);
        // std::print("{:<18} : {}\n", "port", result.port);
        // std::print("{:<18} : {}\n", "tcp test succeeded", result.has_success);

        icmpChecker pinger(ctx);
        pinger.async_check(argv[1]);

        ctx.run();

        icmpCheckResult result = pinger.get_result();

        if (result.ec.value() != 0)
        {
            throw boost::system::system_error(result.ec);
        }

        std::print("{:<20} : {}\n", "hostname", result.hostname);
        std::print("{:<20} : {}\n", "remote address", result.remote_addr);
        std::print("{:<20} : {}ms\n", "reply time", result.reply_time);
        std::print("{:<20} : {}\n", "icmp test succeeded", result.has_success);
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