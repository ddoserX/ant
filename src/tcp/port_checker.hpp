#pragma once

#include <boost/asio.hpp>
#include <string>
#include <print>

namespace asio = boost::asio;
using asio::ip::tcp;
using boost::system::error_code;
using boost::system::system_error;

struct tcpInfo
{
    std::string hostname = "";
    std::string remote_addr = "";
    uint16_t port = 0;
    bool has_success = false;
    boost::system::error_code ec = {};
};

class tcpPortChecker
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

    void async_check(std::string hostname, uint16_t port)
    {
        m_info.hostname = hostname;
        m_info.port = port;
        
        m_resolver.async_resolve(tcp::v4(), hostname, std::to_string(port), 
        [this](const error_code &ec, tcp::resolver::results_type results){
            if (ec.value() != 0)
            {
                m_info.ec = ec;
                return;
            }

            tcp::endpoint ep = {};

            try
            {
                ep = *results.begin();
                m_info.remote_addr = ep.address().to_string();
            }
            catch(const system_error& e)
            {
                m_info.ec = e.code();
                return;
            }
            
            m_sock.open(ep.protocol());
            m_sock.async_connect(ep, std::bind(&tcpPortChecker::handle_connect, this, asio::placeholders::error));
        });
    }

    tcpInfo get_result()
    {
        return m_info;
    }

private:
    void handle_connect(const error_code &ec)
    {
        if (ec.value() != 0)
        {
            m_info.ec = ec;
            return;
        }

        m_info.has_success = true;
    }

    tcp::socket m_sock;
    tcp::resolver m_resolver;
    tcpInfo m_info{};
};