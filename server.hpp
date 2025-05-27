#pragma once
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <iomanip>

using boost::asio::ip::tcp;

class AsyncTCPServer {
public:
    AsyncTCPServer(boost::asio::io_context& io_context, short port);
    void start_accept();
    
private:
    void handle_accept(tcp::socket socket, const boost::system::error_code& error);
    void handle_read(tcp::socket& socket, boost::asio::streambuf& buffer, 
                    const boost::system::error_code& error, size_t bytes_transferred);
    void calculate_average(tcp::socket& socket, const std::string& data);
    
    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    
};
class SyncTCPServer {
public:
    std::string process_request(const std::string& request);  
};
