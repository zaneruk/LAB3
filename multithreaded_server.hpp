#include "multithreaded_server.hpp"
#include <iostream>
#include <sstream>
#include <numeric>
#include <thread>

Session::Session(tcp::socket socket, 
                 boost::asio::strand<boost::asio::io_context::executor_type> strand,
                 std::vector<std::string>& log)
    : socket_(std::move(socket)), strand_(std::move(strand)), log_(log) {}

void Session::start() {
    do_read();
}

void Session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string request(data_, length);
                    handle_request(request);
                }
            }));
}

void Session::handle_request(const std::string& request) {
    // Вычисление факториала как долгой операции
    try {
        int number = std::stoi(request);
        uint64_t result = 1;
        
        // Имитация долгой операции
        for (int i = 2; i <= number; ++i) {
            result *= i;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::string response = "Factorial: " + std::to_string(result) + "\n";
        
        // Логирование через strand
        log_message("Processed request: " + request + " -> " + std::to_string(result));
        
        do_write(response);
    } catch (...) {
        std::string response = "Invalid input\n";
        log_message("Invalid request: " + request);
        do_write(response);
    }
}

void Session::do_write(const std::string& response) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(response),
        boost::asio::bind_executor(strand_,
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            }));
}

void Session::log_message(const std::string& message) {
    boost::asio::post(strand_,
        [this, message]() {
            log_.push_back(message);
            std::cout << "Logged: " << message << std::endl;
        });
}

MultithreadedServer::MultithreadedServer(boost::asio::io_context& io_context, 
                                       short port, int thread_pool_size)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      strand_(boost::asio::make_strand(io_context)),
      thread_pool_size_(thread_pool_size) {
    do_accept();
}

void MultithreadedServer::start_accept() {
    do_accept();
}

void MultithreadedServer::do_accept() {
    acceptor_.async_accept(
        boost::asio::make_strand(io_context_),
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), strand_, log_)->start();
            }
            do_accept();
        });
}