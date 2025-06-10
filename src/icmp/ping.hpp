#pragma once

//
// ping.hpp
// ~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <istream>
#include <ostream>
#include <print>

#include "icmp_header.hpp"
#include "icmp_info.hpp"
#include "ipv4_header.hpp"

namespace asio = boost::asio;
namespace chrono = asio::chrono;

using asio::steady_timer;
using asio::error::operation_aborted;
using asio::ip::icmp;
using boost::system::error_code;

class icmpChecker
{
public:
    icmpChecker(boost::asio::io_context &ctx, icmp::endpoint &ep, std::shared_ptr<icmpInfo> info)
        : m_resolver(ctx), m_sock(ctx, icmp::v4()), m_ep(ep),
          m_timer(ctx), m_info(info), m_sequence(0), m_repliec(0)
    {
        // m_ep = *m_resolver.resolve(icmp::v4(), hostname, "").begin();
        // m_info->hostname = hostname;
        // m_info->remote_addr = m_ep.address().to_string();
    }

    ~icmpChecker()
    {
        stop();
    }

    void start()
    {
        start_send();
        start_receive();
    }

    void stop()
    {
        if (!m_is_stopped)
        {
            m_is_stopped = true;
            m_timer.cancel();
            m_sock.cancel();

            if(m_repliec > 0)
            {
                m_info->has_success = true;
            }

            if (m_info->reply_time != 0)
            {
                m_info->reply_time = m_info->reply_time / m_sequence;
            }
        }
    }

    bool is_stopped()
    {
        return m_is_stopped;
    }

private:
    void start_send()
    {
        if (m_sequence == m_sequence_max)
        {
            // std::print("stop enter\n");
            stop();
            return;
        }

        const int64_t request_timeout = 3;
        std::string body("\"Hello!\"");

        // Create an ICMP header for an echo request.
        icmpHeader echo_request;
        echo_request.type(icmpHeader::echo_request);
        echo_request.code(0);
        echo_request.identifier(get_identifier());
        echo_request.sequence_number(++m_sequence);
        compute_checksum(echo_request, body.begin(), body.end());

        // Encode the request packet.
        boost::asio::streambuf request_buffer;
        std::ostream os(&request_buffer);
        os << echo_request << body;

        // Send the request.
        m_time_sent = steady_timer::clock_type::now();
        m_sock.send_to(request_buffer.data(), m_ep);

        // Wait up to five seconds for a reply.
        m_repliec = 0;
        m_timer.expires_at(m_time_sent + chrono::seconds(request_timeout));
        m_timer.async_wait(std::bind(&icmpChecker::handle_timeout, this));
    }

    void handle_timeout()
    {
        if (m_is_stopped)
        {
            // std::print("handle_timeout aborted\n");
            return;
        }

        if (m_repliec == 0)
        {
            // std::print("Request timed out\nn");
        }

        const int64_t request_timeout = 1;

        // Requests must be sent no less than one second apart.
        m_timer.expires_at(m_time_sent + chrono::seconds(request_timeout));
        m_timer.async_wait(std::bind(&icmpChecker::start_send, this));
    }

    void start_receive()
    {
        if (m_is_stopped)
        {
            return;
        }

        // Discard any data already in the buffer.
        m_reply_buf.consume(m_reply_buf.size());

        // Wait for a reply. We prepare the buffer to receive up to 64KB.
        m_sock.async_receive(m_reply_buf.prepare(65536),
                             std::bind(&icmpChecker::handle_receive, this, asio::placeholders::bytes_transferred));
    }

    void handle_receive(std::size_t length)
    {
        if (m_is_stopped)
        {
            // std::print("handle_receive aborted\n");
            return;
        }

        // The actual number of bytes received is committed to the buffer so that we
        // can extract it using a std::istream object.
        m_reply_buf.commit(length);

        // Decode the reply packet.
        std::istream is(&m_reply_buf);
        ipv4Header ipv4_hdr;
        icmpHeader icmp_hdr;
        is >> ipv4_hdr >> icmp_hdr;

        // We can receive all ICMP packets received by the host, so we need to
        // filter out only the echo replies that match the our identifier and
        // expected sequence number.
        if (is && icmp_hdr.type() == icmpHeader::echo_reply && icmp_hdr.identifier() == get_identifier() && icmp_hdr.sequence_number() == m_sequence)
        {
            // If this is the first reply, interrupt the five second timeout.
            if (m_repliec++ == 0)
                m_timer.cancel();

            // Print out some information about the reply packet.
            chrono::steady_clock::time_point now = chrono::steady_clock::now();
            chrono::steady_clock::duration elapsed = now - m_time_sent;
            // std::print("{:d} bytes from {:s}: icmp_seq={:d}, ttl={:d}, time={:d}ms\n",
            //            length - ipv4_hdr.header_length(),
            //            ipv4_hdr.source_address().to_string(),
            //            icmp_hdr.sequence_number(),
            //            ipv4_hdr.time_to_live(),
            //            chrono::duration_cast<chrono::milliseconds>(elapsed).count());
            
            m_info->reply_time += chrono::duration_cast<chrono::milliseconds>(elapsed).count();
        }

        start_receive();
    }

    static unsigned short get_identifier()
    {
#if defined(BOOST_ASIO_WINDOWS)
        return static_cast<unsigned short>(GetCurrentProcessId());
#else
        return static_cast<unsigned short>(getpid());
#endif
    }

    icmp::resolver m_resolver;
    icmp::socket m_sock;
    icmp::endpoint m_ep;
    steady_timer m_timer;
    std::shared_ptr<icmpInfo> m_info;
    uint16_t m_sequence;
    chrono::steady_clock::time_point m_time_sent;
    boost::asio::streambuf m_reply_buf;
    std::size_t m_repliec;

    bool m_is_stopped = false;
    uint16_t m_sequence_max = 2;
};
