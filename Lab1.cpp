#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

int main() {
    setlocale(LC_ALL, "Russian");

    try {
        boost::asio::io_context io;

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "12345");

        tcp::socket socket(io);
        boost::asio::connect(socket, endpoints);

        std::cout << "Клиент подключился к серверу." << std::endl;
        std::cout << "Команды:" << std::endl;
        std::cout << "  обычная строка        -> сервер вернёт её в верхнем регистре" << std::endl;
        std::cout << "  sum 10 20 30          -> сервер посчитает сумму" << std::endl;
        std::cout << "  ping                  -> сервер ответит pong через 3 секунды" << std::endl;
        std::cout << "  ping 5                -> сервер ответит pong через 5 секунд" << std::endl;
        std::cout << "  exit                  -> выход" << std::endl;

        while (true) {
            std::string message;

            std::cout << "\nВведите сообщение: ";
            std::getline(std::cin, message);

            if (message.empty()) {
                std::cout << "Пустое сообщение не отправляется." << std::endl;
                continue;
            }

            message += "\n";

            boost::asio::write(socket, boost::asio::buffer(message));

            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, '\n');

            std::istream input(&buffer);
            std::string response;
            std::getline(input, response);

            std::cout << "Ответ сервера: " << response << std::endl;

            if (message == "exit\n") {
                break;
            }
        }

    } catch (std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
    }

    return 0;
}
