#include "multithreaded_server.hpp"
#include <iostream>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: server <port> <thread_pool_size>\n";
            return 1;
        }
        
        short port = std::atoi(argv[1]);
        int thread_pool_size = std::atoi(argv[2]);
        
        boost::asio::io_context io_context;
        MultithreadedServer server(io_context, port, thread_pool_size);
        
        // Создаем пул потоков
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_pool_size; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }
        
        std::cout << "Server started on port " << port 
                  << " with " << thread_pool_size << " threads\n";
        
        // Ждем завершения всех потоков
        for (auto& thread : threads) {
            thread.join();
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    return 0;
}