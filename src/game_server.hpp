/** @file game_server.hpp
Заголовочный файл класса многопроцессного TCP-сервера игры "Лабиринт"
* @details Сервер принимает входящие TCP-подключения, для каждого клиента порождает дочерний процесс (fork),
в котором запускается GameSession. Поддерживает одновременную работу с неограниченным числом игроков
*/

#ifndef GAME_SERVER_HPP
#define GAME_SERVER_HPP

#include <string>
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "game_session.hpp"
#include "maze_protocol.hpp"

/** Класс многопроцессного TCP-сервера
 * @details Основной метод run() в бесконечном цикле принимает подключения и для каждого порождает дочерний процесс через fork()
 */
class GameServer
{
public:
    /** Создаёт сервер на заданном порту
     * @param port Порт для прослушивания
     * @param max_moves Лимит ходов в обычном режиме
     * @param maze_size Размер стороны квадратного лабиринта
     * @param wall_count Число генерируемых стен
     */
    GameServer(unsigned short port = 4321, int max_moves = 10, unsigned maze_size = 3u, unsigned wall_count = 5u)
        : _port(port), _max_moves(max_moves), _maze_size(maze_size), _wall_count(wall_count), _listen_socket(-1)
    {
    }

    /** Удаляет сервер, закрывая слушающий сокет */
    ~GameServer()
    {
        if (_listen_socket >= 0)
        {
            close(_listen_socket);
        }
    }

    /** Запускает сервер
     * @throw std::runtime_error При ошибках сокета
     */
    void run();

private:
    /** Настраивает слушающий сокет: создание, bind, listen */
    void setupSocket();

    /** Обработчик сигнала SIGCHLD — собирает завершившиеся дочерние процессы
     * @param signum Номер сигнала (не используется)
     */
    static void sigchldHandler(int signum);

    unsigned short _port; ///< Порт прослушивания
    int _max_moves;       ///< Лимит ходов для обычного режима
    unsigned _maze_size;  ///< Размер стороны лабиринта
    unsigned _wall_count; ///< Число генерируемых стен
    int _listen_socket;   ///< Слушающий TCP-сокет
};

void GameServer::run()
{
    // Регистрация обработчика SIGCHLD
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, nullptr) == -1)
    {
        throw std::runtime_error("GameServer: sigaction failed");
    }

    setupSocket();

    std::cout << "[server] Запущен на порту " << _port
              << ", лимит ходов: " << _max_moves
              << ", размер: " << _maze_size << "x" << _maze_size
              << ", стен: " << _wall_count << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        const int client_socket = accept(_listen_socket, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
        if (client_socket < 0)
        {
            // accept мог быть прерван сигналом — продолжаем
            if (errno == EINTR)
            {
                continue;
            }
            std::cerr << "[server] ошибка accept: " << std::strerror(errno) << std::endl;
            continue;
        }

        const std::string client_ip = inet_ntoa(client_addr.sin_addr);
        std::cout << "[server] Подключение от " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

        const pid_t pid = fork();
        if (pid < 0)
        {
            std::cerr << "[server] ошибка fork: " << std::strerror(errno) << std::endl;
            close(client_socket);
            continue;
        }

        if (pid == 0)
        {
            // Дочерний процесс
            close(_listen_socket); // дочерний процесс не принимает новые подключения
            try
            {
                GameSession session(client_socket, _max_moves, _maze_size, _wall_count);
                session.run();
            }
            catch (const std::exception &e)
            {
                std::cerr << "[session] ошибка: " << e.what() << std::endl;
            }
            close(client_socket);
            _exit(0); // завершаем дочерний процесс
        }
        else
        {
            // Родительский процесс
            close(client_socket); // родитель не работает с клиентом
        }
    }
}

void GameServer::setupSocket()
{
    _listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_socket < 0)
    {
        throw std::runtime_error("GameServer: socket creation failed");
    }

    // Переиспользование адреса для быстрого перезапуска
    const int optval = 1;
    setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_listen_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        throw std::runtime_error("GameServer: bind failed");
    }

    if (listen(_listen_socket, SOMAXCONN) < 0)
    {
        throw std::runtime_error("GameServer: listen failed");
    }
}

void GameServer::sigchldHandler(int)
{
    // Сбор всех завершившихся дочерних процессов (неблокирующий)
    while (waitpid(-1, nullptr, WNOHANG) > 0)
    {
    }
}

#endif
