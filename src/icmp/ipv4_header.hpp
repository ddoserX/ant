#pragma once

#include <boost/asio.hpp>
#include "icmp_header.hpp"

struct ipv4_header
{
    unsigned char version_and_header_length;
    unsigned char type_of_service;
    unsigned short total_length;
    unsigned short identification;
    unsigned short fragment_offset;
    unsigned char time_to_live;
    unsigned char protocol;
    unsigned short header_checksum;
    boost::asio::ip::address_v4 source_address;
    boost::asio::ip::address_v4 destination_address;

    unsigned int header_length() const
    {
        return (version_and_header_length & 0x0F) * 4;
    }
};

std::istream& operator>>(std::istream& is, ipv4_header& hdr)
{
    is.read(reinterpret_cast<char*>(&hdr.version_and_header_length), 1);
    is.read(reinterpret_cast<char*>(&hdr.type_of_service), 1);
    is.read(reinterpret_cast<char*>(&hdr.total_length), 2);
    hdr.total_length = ntohs(hdr.total_length); // Преобразование порядка байтов

    is.read(reinterpret_cast<char*>(&hdr.identification), 2);
    is.read(reinterpret_cast<char*>(&hdr.fragment_offset), 2);
    is.read(reinterpret_cast<char*>(&hdr.time_to_live), 1);
    is.read(reinterpret_cast<char*>(&hdr.protocol), 1);
    is.read(reinterpret_cast<char*>(&hdr.header_checksum), 2);
    is.read(reinterpret_cast<char*>(&hdr.source_address), 4);
    is.read(reinterpret_cast<char*>(&hdr.destination_address), 4);

    return is;
}

std::istream& operator>>(std::istream& is, icmp_header& hdr)
{
    is.read(reinterpret_cast<char*>(&hdr.type), 1);
    is.read(reinterpret_cast<char*>(&hdr.code), 1);
    is.read(reinterpret_cast<char*>(&hdr.checksum), 2);
    is.read(reinterpret_cast<char*>(&hdr.identifier), 2);
    is.read(reinterpret_cast<char*>(&hdr.sequence_number), 2);
    
    // Преобразование порядка байтов для входящих данных
    hdr.checksum = ntohs(hdr.checksum);
    hdr.identifier = ntohs(hdr.identifier);
    hdr.sequence_number = ntohs(hdr.sequence_number);
    return is;
}