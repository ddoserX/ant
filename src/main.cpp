#include <print>
#include <boost/asio.hpp>

namespace asio = boost::asio;

int main()
{
    std::print("hello from boost.asio\n");
    asio::io_context ctx;
    ctx.run();
}
