#include <boost/program_options.hpp>
#include <iostream>

#include "checker.hpp"
#include "icmp/icmp_cheker.hpp"
#include "tcp/port_checker.hpp"

namespace asio = boost::asio;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	uint16_t port = 0;
	std::string hostname = "google.com";

	po::options_description desc("command list");
	// clang-format off
	desc.add_options()
		("help,h,", "print help information")
		("host,H", po::value<std::string>(&hostname), "target hostname for tcp or icmp check")
		("port,P", po::value<uint16_t>(&port), "target port for tcp check");
	// clang-format on

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << '\n';
			return 1;
		}

		asio::io_context ctx;
		checkData data(hostname, port);

		std::unique_ptr<IChecker> checker;

		if (port == 0) {
			checker = std::make_unique<icmpChecker>(ctx);
		} else {
			checker = std::make_unique<tcpPortChecker>(ctx);
		}

		checker->async_check(data);

		ctx.run();

		error_code ec = checker->has_error();
		if (ec.value() != 0) {
			throw system_error(ec);
		}

		checker->print(std::cout);
		std::cout << std::endl;
	} catch (const boost::system::system_error& e) {
		std::cout << e.code().message() << '\n';
	} catch (const std::exception& e) {
		std::cout << e.what() << '\n';
	} catch (...) {
		std::cout << "Unknown error\n";
	}

	return 0;
}