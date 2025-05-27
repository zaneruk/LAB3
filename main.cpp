#include "server.hpp"
#include "client.hpp"
#include <thread>
#include <iostream>

void run_sync_server() {
    try {
        boost::asio::io_context io_context;
        SyncTCPServer server(io_context, 12345);
        
        std::thread server_thread([&server]() {
            server.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        SyncTCPClient client("127.0.0.1", "12345");
        
        std::cout << "5 * 7 = " << client.send_request("5 * 7") << std::endl;
        std::cout << "10 / 4 = " << client.send_request("10 / 4") << std::endl;
        
        server_thread.detach();
    } catch (std::exception& e) {
        std::cerr << "Sync server error: " << e.what() << std::endl;
    }
}

void run_async_server() {
    try {
        boost::asio::io_context io_context;
        AsyncTCPServer server(io_context, 12346);
        
        std::thread server_thread([&io_context]() {
            io_context.run();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        SyncTCPClient client("127.0.0.1", "12346");
        
        std::cout << "Average of 10 20 30: " << client.send_request("10 20 30") << std::endl;
        std::cout << "Reminder test: " << client.send_request("remind 2 Wake up!") << std::endl;
        
        server_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Async server error: " << e.what() << std::endl;
    }
}

void run_multithreaded_server() {
    try {
        boost::asio::io_context io_context;
        MultithreadedServer server(io_context, 12347, 4);
        
        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        SyncTCPClient client("127.0.0.1", "12347");
        
        std::cout << "Factorial of 5: " << client.send_request("5") << std::endl;
        
        for (auto& thread : threads) {
            thread.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Multithreaded server error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Sync Server Test ===" << std::endl;
    run_sync_server();
    
    std::cout << "\n=== Async Server Test ===" << std::endl;
    run_async_server();
    
    std::cout << "\n=== Multithreaded Server Test ===" << std::endl;
    run_multithreaded_server();
    
    return 0;
}