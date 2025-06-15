#pragma once

// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <istream>
#include <ostream>

#include "checker.hpp"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace asio = boost::asio;
namespace chrono = asio::chrono;

using asio::steady_timer;
using asio::error::operation_aborted;
using asio::ip::icmp;
using boost::system::error_code;
using boost::system::system_error;

struct icmpCheckResult {
	std::string hostname = "";
	std::string remote_addr = "";
	int64_t reply_time = 0;
	error_code ec = {};
	bool has_success = false;
};

class icmpChecker : public IChecker
{
    public:
	icmpChecker(boost::asio::io_context& ctx, uint16_t sequence_max = 5)
	    : m_resolver(ctx), m_sock(ctx, icmp::v4()), m_timer(ctx), m_sequence_max(sequence_max)
	{
	}

	~icmpChecker() { stop(); }

	icmpChecker(const icmpChecker&) = delete;
	icmpChecker(icmpChecker&&) = delete;
	icmpChecker& operator=(const icmpChecker&) = delete;
	icmpChecker& operator=(icmpChecker&&) = delete;

	void async_check(const checkData& data) override
	{
		m_result.hostname = data.hostname;

		m_resolver.async_resolve(icmp::v4(), data.hostname, "",
					 [this](const error_code& ec, icmp::resolver::results_type results) {
						 if (ec.value() != 0) {
							 m_result.ec = ec;
							 return;
						 }

						 icmp::endpoint ep = {};

						 try {
							 ep = *results.begin();
							 m_result.remote_addr = ep.address().to_string();

							 start_send(ep);
							 start_receive();
						 } catch (const system_error& se) {
							 m_result.ec = se.code();
						 }
					 });
	}

	void print(std::ostream& os) const override
	{
		std::string formated_string = "";
		formated_string += std::format("{:<20} : {}\n", "hostname", m_result.hostname);
		formated_string += std::format("{:<20} : {}\n", "remote address", m_result.remote_addr);
		formated_string += std::format("{:<20} : {}ms\n", "reply time", m_result.reply_time);
		formated_string += std::format("{:<20} : {}\n", "icmp test succeeded", m_result.has_success);

		os << formated_string;
	}

	error_code has_error() const override { return m_result.ec; }

	void stop()
	{
		if (!m_is_stopped) {
			m_is_stopped = true;
			m_timer.cancel();
			m_sock.cancel();

			if (m_repliec > 0) {
				m_result.has_success = true;
			}

			if (m_result.reply_time != 0) {
				m_result.reply_time = m_result.reply_time / m_sequence;
			}
		}
	}

    private:
	void start_send(const icmp::endpoint& endpoint)
	{
		if (m_sequence == m_sequence_max) {
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
		m_sock.send_to(request_buffer.data(), endpoint);

		// Wait up to five seconds for a reply.
		m_repliec = 0;
		m_timer.expires_at(m_time_sent + chrono::seconds(request_timeout));
		m_timer.async_wait(std::bind(&icmpChecker::handle_timeout, this, endpoint));
	}

	void handle_timeout(const icmp::endpoint& endpoint)
	{
		if (m_is_stopped) {
			return;
		}

		// const int64_t request_timeout = 1;

		// // Requests must be sent no less than one second apart.
		// m_timer.expires_at(m_time_sent +
		// chrono::seconds(request_timeout));
		// m_timer.async_wait(std::bind(&icmpChecker::start_send, this,
		// endpoint));
		start_send(endpoint);
	}

	void start_receive()
	{
		if (m_is_stopped) {
			return;
		}

		// Discard any data already in the buffer.
		m_reply_buf.consume(m_reply_buf.size());

		// Wait for a reply. We prepare the buffer to receive up to
		// 64KB.
		m_sock.async_receive(m_reply_buf.prepare(65536), std::bind(&icmpChecker::handle_receive, this,
									   asio::placeholders::bytes_transferred));
	}

	void handle_receive(std::size_t length)
	{
		if (m_is_stopped) {
			return;
		}

		// The actual number of bytes received is committed to the
		// buffer so that we can extract it using a std::istream object.
		m_reply_buf.commit(length);

		// Decode the reply packet.
		std::istream is(&m_reply_buf);
		ipv4Header ipv4_hdr;
		icmpHeader icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we
		// need to filter out only the echo replies that match the our
		// identifier and expected sequence number.
		if (is && icmp_hdr.type() == icmpHeader::echo_reply && icmp_hdr.identifier() == get_identifier() &&
		    icmp_hdr.sequence_number() == m_sequence) {
			// If this is the first reply, interrupt the five second
			// timeout.
			if (m_repliec++ == 0) m_timer.cancel();

			// Print out some information about the reply packet.
			chrono::steady_clock::time_point now = chrono::steady_clock::now();
			chrono::steady_clock::duration elapsed = now - m_time_sent;

			m_result.reply_time += chrono::duration_cast<chrono::milliseconds>(elapsed).count();
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
	steady_timer m_timer;
	chrono::steady_clock::time_point m_time_sent{};
	boost::asio::streambuf m_reply_buf{};

	icmpCheckResult m_result{};

	size_t m_repliec = 0;
	uint16_t m_sequence_max = 5;
	uint16_t m_sequence = 0;
	bool m_is_stopped = false;
};
