// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <algorithm>
#include <istream>
#include <ostream>

// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     type      |     code      |          checksum            |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          identifier           |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

class icmpHeader
{
    public:
	enum {
		echo_reply = 0,
		destination_unreachable = 3,
		source_quench = 4,
		redirect = 5,
		echo_request = 8,
		time_exceeded = 11,
		parameter_problem = 12,
		timestamp_request = 13,
		timestamp_reply = 14,
		info_request = 15,
		info_reply = 16,
		address_request = 17,
		address_reply = 18
	};

	icmpHeader() { std::fill(rep_, rep_ + sizeof(rep_), static_cast<unsigned char>(0)); }

	unsigned char type() const { return rep_[0]; }
	unsigned char code() const { return rep_[1]; }
	unsigned short checksum() const { return decode(2, 3); }
	unsigned short identifier() const { return decode(4, 5); }
	unsigned short sequence_number() const { return decode(6, 7); }

	void type(unsigned char n) { rep_[0] = n; }
	void code(unsigned char n) { rep_[1] = n; }
	void checksum(unsigned short n) { encode(2, 3, n); }
	void identifier(unsigned short n) { encode(4, 5, n); }
	void sequence_number(unsigned short n) { encode(6, 7, n); }

	friend std::istream& operator>>(std::istream& is, icmpHeader& header)
	{
		return is.read(reinterpret_cast<char*>(header.rep_), 8);
	}

	friend std::ostream& operator<<(std::ostream& os, const icmpHeader& header)
	{
		return os.write(reinterpret_cast<const char*>(header.rep_), 8);
	}

    private:
	unsigned short decode(int a, int b) const { return static_cast<unsigned short>((rep_[a] << 8) + rep_[b]); }

	void encode(int a, int b, unsigned short n)
	{
		rep_[a] = static_cast<unsigned char>(n >> 8);
		rep_[b] = static_cast<unsigned char>(n & 0xFF);
	}

	unsigned char rep_[8];
};

template <typename Iterator>
void compute_checksum(icmpHeader& header, Iterator body_begin, Iterator body_end)
{
	unsigned int sum = (static_cast<unsigned int>(header.type()) << static_cast<unsigned int>(8)) +
			   static_cast<unsigned int>(header.code()) + static_cast<unsigned int>(header.identifier()) +
			   static_cast<unsigned int>(header.sequence_number());

	Iterator body_iter = body_begin;
	while (body_iter != body_end) {
		sum += (static_cast<unsigned char>(*body_iter++) << 8);
		if (body_iter != body_end) sum += static_cast<unsigned char>(*body_iter++);
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	header.checksum(static_cast<unsigned short>(~sum));
}