#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <numeric>

// Для ntohs/htons на не-Windows системах
#if !defined(BOOST_ASIO_WINDOWS)
#include <arpa/inet.h>
#include <unistd.h> // Для getpid()
#endif

#include "icmp/icmp_header.hpp"
#include "icmp/ipv4_header.hpp"

class pinger
{
public:
    pinger(boost::asio::io_context& io_context, const char* host)
        : resolver_(io_context), socket_(io_context, boost::asio::ip::icmp::v4()),
          timer_(io_context), sequence_number_(0), num_replies_(0)
    {
        io_context_ = &io_context; 
        
        // Генерируем ID один раз при создании пингера.
        // Используем PID процесса для начального значения, беря младшие 16 бит.
        my_identifier_ = static_cast<unsigned short>(::getpid() & 0xFFFF); 

        destination_ = *resolver_.resolve(boost::asio::ip::icmp::v4(), host, "").begin();

        std::cout << "Pinging " << host << " (" << destination_.address().to_string() << ")"
                  << " with ID: " << my_identifier_ << std::endl;

        start_send();
        start_receive();
    }

private:
    void start_send()
    {
        std::string body("Hello from Boost.Asio ICMP ping.");

        icmp_header echo_request;
        echo_request.type = icmp_header::echo_request;
        echo_request.code = 0;
        echo_request.identifier = htons(my_identifier_); // ИСПОЛЬЗУЕМ СОХРАНЕННЫЙ ID и преобразуем в сетевой порядок байтов
        echo_request.sequence_number = htons(sequence_number_++); // Увеличиваем номер последовательности, сразу в сетевой порядок байтов

        // Для вычисления контрольной суммы, поле checksum должно быть равно 0.
        // Мы используем временную копию заголовка для этого.
        icmp_header temp_echo_request = echo_request; 
        temp_echo_request.checksum = 0; 

        // Копируем временный заголовок (с обнуленной КС) и тело сообщения в буфер для вычисления КС.
        std::vector<char> data_for_checksum(sizeof(icmp_header) + body.size());
        std::memcpy(data_for_checksum.data(), &temp_echo_request, sizeof(icmp_header));
        std::memcpy(data_for_checksum.data() + sizeof(icmp_header), body.data(), body.size());
        
        // Вычисляем контрольную сумму и присваиваем ее оригинальному заголовку.
        echo_request.checksum = icmp_header::compute_checksum(data_for_checksum.data(), data_for_checksum.size());

        // Записываем финальный пакет (заголовок с корректной КС + тело) в буфер для отправки.
        request_buffer_.consume(request_buffer_.size()); // Очищаем буфер, если что-то уже есть
        std::ostream os_final(&request_buffer_);
        os_final.write(reinterpret_cast<const char*>(&echo_request), sizeof(icmp_header)); 
        os_final << body;

        time_sent_ = std::chrono::steady_clock::now();
        socket_.send_to(request_buffer_.data(), destination_);
        std::cout << "Sent packet " << (sequence_number_ - 1) << " with ID " << my_identifier_ << std::endl;

        // Ждем таймаута или следующего пакета.
        timer_.expires_at(time_sent_ + std::chrono::seconds(1)); // 1 секунда между пакетами
        timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
    }

    void handle_timeout()
    {
        if (num_replies_ == 0)
        {
            std::cout << "Request timed out" << std::endl;
        }

        // Отправляем следующий пакет, если не все отправлены.
        if (sequence_number_ < 4) // Отправляем 4 пакета
        {
            num_replies_ = 0; // Сбрасываем для нового пакета
            start_send();
        }
        else
        {
            std::cout << "Ping finished." << std::endl;
            // Останавливаем io_context, когда все пинги завершены.
            io_context_->stop(); 
        }
    }

    void start_receive()
    {
        // Отбрасываем любые данные, уже находящиеся в буфере.
        reply_buffer_.consume(reply_buffer_.size());

        // Ждем ответа. Подготавливаем буфер для приема до 64 КБ.
        socket_.async_receive(reply_buffer_.prepare(65536),
                              boost::bind(&pinger::handle_receive, this,
                                          boost::asio::placeholders::error,
                                          boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive(const boost::system::error_code& error, std::size_t length)
    {
        if (!error)
        {
            // Фактическое количество полученных байтов фиксируется в буфере.
            reply_buffer_.commit(length);

            std::istream is(&reply_buffer_);
            ipv4_header ipv4_hdr;
            icmp_header icmp_hdr;
            
            // Пытаемся прочитать заголовки
            if (! (is >> ipv4_hdr) ) {
                std::cerr << "Failed to parse IPv4 header." << std::endl;
                start_receive(); // Продолжаем ждать следующие ответы
                return;
            }
            if (! (is >> icmp_hdr) ) {
                std::cerr << "Failed to parse ICMP header." << std::endl;
                start_receive(); // Продолжаем ждать следующие ответы
                return;
            }

            // Отладочный вывод
            // std::cout << "IPv4 header length: " << ipv4_hdr.header_length() << " bytes" << std::endl;
            // std::cout << "ICMP type: " << static_cast<int>(icmp_hdr.type) << ", code: " << static_cast<int>(icmp_hdr.code) << std::endl;
            // std::cout << "Received: id=" << icmp_hdr.identifier
            //           << ", seq=" << icmp_hdr.sequence_number
            //           << ", expected_id=" << my_identifier_ 
            //           << ", expected_seq=" << (sequence_number_ - 1) << std::endl;

            // Фильтруем пакеты: должен быть Echo Reply, с нашим ID и ожидаемым Sequence Number
            if (icmp_hdr.type == icmp_header::echo_reply &&
                icmp_hdr.identifier == my_identifier_ && 
                icmp_hdr.sequence_number == (sequence_number_ - 1))
            {
                if (num_replies_++ == 0)
                {
                    timer_.cancel(); // Отменяем таймер, если это первый ответ на текущий запрос
                }

                std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_sent_);
                std::cout << length - ipv4_hdr.header_length() << " bytes from "
                          << ipv4_hdr.source_address.to_string()
                          << ": icmp_seq=" << icmp_hdr.sequence_number
                          << ", ttl=" << static_cast<int>(ipv4_hdr.time_to_live)
                          << ", time=" << elapsed.count() << " ms" << std::endl;
            } 
            // else 
            // {
            //     std::cout << "Packet filtered out. Reason: "
            //               << "type_match=" << (icmp_hdr.type == icmp_header::echo_reply) << " (expected " << icmp_header::echo_reply << "), "
            //               << "id_match=" << (icmp_hdr.identifier == my_identifier_) << ", "
            //               << "seq_match=" << (icmp_hdr.sequence_number == (sequence_number_ - 1)) << std::endl;
            // }
        }
        else if (error != boost::asio::error::operation_aborted)
        {
            std::cerr << "Receive error: " << error.message() << std::endl;
        }

        start_receive(); // Продолжаем ждать следующие ответы
    }
    
    boost::asio::io_context* io_context_; 
    boost::asio::ip::icmp::resolver resolver_;
    boost::asio::ip::icmp::endpoint destination_;
    boost::asio::ip::icmp::socket socket_;
    boost::asio::steady_timer timer_;
    std::chrono::steady_clock::time_point time_sent_;
    boost::asio::streambuf request_buffer_; // Буфер для отправки запросов
    boost::asio::streambuf reply_buffer_;   // Буфер для приема ответов
    unsigned short sequence_number_;
    std::size_t num_replies_;
    
    unsigned short my_identifier_; // Член данных для хранения идентификатора, который мы используем
};

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <host>" << std::endl;
        std::cerr << "Example: " << argv[0] << " google.com" << std::endl;
#if !defined(BOOST_ASIO_WINDOWS)
        std::cerr << "(On Linux, you may need to run this program as root or set net.ipv4.ping_group_range.)" << std::endl;
#endif
        return 1;
    }

    try
    {
        boost::asio::io_context io_context;
        pinger p(io_context, argv[1]);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
#if !defined(BOOST_ASIO_WINDOWS)
        std::cerr << "(On Linux, this often means you need to run as root or configure net.ipv4.ping_group_range.)" << std::endl;
#endif
    }

    return 0;
}