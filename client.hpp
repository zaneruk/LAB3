#pragma once
#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;

class SyncTCPClient {
public:
    SyncTCPClient(const std::string& server, const std::string& port);
    std::string send_request(const std::string& request);

private:
    boost::asio::io_context io_context_;
    tcp::resolver resolver_;
};