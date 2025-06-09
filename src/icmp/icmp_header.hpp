#pragma once

#include <cstddef>

struct icmp_header
{
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short identifier;
    unsigned short sequence_number;

    enum { echo_request = 8, echo_reply = 0 };

    // Вычисление контрольной суммы
    static unsigned short compute_checksum(const void* data, std::size_t size)
    {
        unsigned long sum = 0;
        const unsigned short* ptr = static_cast<const unsigned short*>(data);

        // Суммируем 16-битные слова
        while (size > 1)
        {
            sum += *ptr++;
            size -= 2;
        }

        // Если остался один байт, добавить его (дополнив нулями до 16 бит)
        if (size == 1)
        {
            sum += *reinterpret_cast<const unsigned char*>(ptr);
        }

        // Добавляем переполнения, пока сумма не поместится в 16 бит
        while (sum >> 16)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }

        // Возвращаем побитовое дополнение
        return static_cast<unsigned short>(~sum);
    }
};