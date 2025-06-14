#pragma once

#include <boost/asio.hpp>
#include <string>
#include <format>

#include "checker.hpp"

namespace asio = boost::asio;
using asio::ip::tcp;
using boost::system::error_code;
using boost::system::system_error;

struct tcpCheckResult
{
    std::string hostname = "";
    std::string remote_addr = "";
    error_code ec = {};
    uint16_t port = 0;
    bool has_success = false;
};

class tcpPortChecker : public IChecker
{
public:
    tcpPortChecker(asio::io_context &ctx)
        : m_sock(ctx), m_resolver(ctx)
    {
    }

    ~tcpPortChecker()
    {
        if (m_sock.is_open())
        {
            m_sock.close();
        }
    }

    tcpPortChecker(const tcpPortChecker &) = delete;
    tcpPortChecker(tcpPortChecker &&) = delete;
    tcpPortChecker &operator=(const tcpPortChecker &) = delete;
    tcpPortChecker &operator=(tcpPortChecker &&) = delete;

    void async_check(const checkData &data) override
    {
        m_result.hostname = data.hostname;
        m_result.port = data.port;
        
        m_resolver.async_resolve(tcp::v4(), data.hostname, std::to_string(data.port), 
        [this](const error_code &ec, tcp::resolver::results_type results){
            if (ec.value() != 0)
            {
                m_result.ec = ec;
                return;
            }

            tcp::endpoint ep = {};

            try
            {
                ep = *results.begin();
                m_result.remote_addr = ep.address().to_string();

                m_sock.open(ep.protocol());
                m_sock.async_connect(ep, std::bind(&tcpPortChecker::handle_connect, this, asio::placeholders::error));
            }
            catch(const system_error& se)
            {
                m_result.ec = se.code();
            }
        });
    }

    void print(std::ostream &os) const override
    {
        std::string formated_string = "";
        formated_string += std::format("{:<20} : {}\n", "hostname", m_result.hostname);
        formated_string += std::format("{:<20} : {}\n", "remote address", m_result.remote_addr);
        formated_string += std::format("{:<20} : {}\n", "reply time", m_result.port);
        formated_string += std::format("{:<20} : {}\n", "tcp test succeeded", m_result.has_success);

        os << formated_string;
    }

    error_code has_error() const override
    {
        return m_result.ec;
    }

private:
    void handle_connect(const error_code &ec)
    {
        if (ec.value() != 0)
        {
            m_result.ec = ec;
            return;
        }

        m_result.has_success = true;
    }

    tcp::socket m_sock;
    tcp::resolver m_resolver;
    tcpCheckResult m_result{};
};