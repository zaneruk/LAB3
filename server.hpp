#pragma once
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <iomanip>

using boost::asio::ip::tcp;

class SyncTCPServer {
public:
    SyncTCPServer(boost::asio::io_context& io_context, short port);
    void start();

private:
    void handle_client(tcp::socket socket);
    std::string process_request(const std::string& request);
    
    tcp::acceptor acceptor_;
};

class AsyncTCPServer {
public:
    AsyncTCPServer(boost::asio::io_context& io_context, short port);
    void start_accept();
    
private:
    void handle_accept(tcp::socket socket, const boost::system::error_code& error);
    void handle_read(tcp::socket& socket, boost::asio::streambuf& buffer, 
                    const boost::system::error_code& error, size_t bytes_transferred);
    void calculate_average(tcp::socket& socket, const std::string& data);
    void handle_reminder(tcp::socket& socket, int seconds, const std::string& reminder);
    
    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, 
            boost::asio::strand<boost::asio::io_context::executor_type> strand,
            std::vector<std::string>& log);
    void start();

private:
    void do_read();
    void handle_request(const std::string& request);
    void do_write(const std::string& response);
    void log_message(const std::string& message);
    
    tcp::socket socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::vector<std::string>& log_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class MultithreadedServer {
public:
    MultithreadedServer(boost::asio::io_context& io_context, short port, int thread_pool_size);
    void start_accept();

private:
    void do_accept();
    
    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::vector<std::string> log_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    int thread_pool_size_;
};