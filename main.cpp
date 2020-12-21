#include <iostream>
#include <memory>
#include <ctime>
#include "include/buffer.hpp"

bool menu(std::unique_ptr<buffer>& b) {
    std::cout <<
              "1. Insert string" << std::endl <<
              "2. Edit string" << std::endl <<
              "3. Delete string" << std::endl <<
              std::endl <<
              "4. View buffer" << std::endl <<
              std::endl <<
              "5. Save as .txt" << std::endl <<
              "6. Load .txt" << std::endl <<
              std::endl <<
              "7. Exit" << std::endl;

    unsigned short action = 0;

    std::cin >> action;
    std::cin.ignore();

    if (action == 1) {
        std::cout << "Input string:" << std::endl;

        std::string string_;
        std::getline(std::cin, string_);

        std::cout << "Input position:" << std::endl;

        size_t position;
        std::cin >> position;

        clock_t start = clock();

        (*b).insert(position, string_);

        clock_t end = clock();
        std::cout << "Operation duration: " << (double)(end - start) / CLOCKS_PER_SEC << std::endl;

        return true;
    }
    else if (action == 2) {
        std::cout << "Input position:" << std::endl;

        size_t position;
        std::cin >> position;
        std::cin.ignore();

        std::cout << "Input new value:" << std::endl;
        std::string string_;
        std::getline(std::cin, string_);

        clock_t start = clock();

        (*b)[position] = string_;

        clock_t end = clock();
        std::cout << "Operation duration: " << (double)(end - start) / CLOCKS_PER_SEC << std::endl;

        return true;
    }
    else if (action == 3) {
        std::cout << "Input position:" << std::endl;

        size_t position;
        std::cin >> position;

        clock_t start = clock();

        (*b).erase(position);

        clock_t end = clock();
        std::cout << "Operation duration: " << (double)(end - start) / CLOCKS_PER_SEC << std::endl;

        return true;
    }

    else if (action == 4) {
        size_t counter = 0;
        for (auto str : (*b)) {
            std::cout << counter++ << ". " << str << std::endl;
        }

        std::cout << std::endl;

        return true;
    }

    else if (action == 5) {
        std::cout << "Input filename (with .txt):" << std::endl;
        std::string filename;
        std::getline(std::cin, filename);

        std::fstream text(filename, std::ios::out | std::ios::trunc);

        if (!text.is_open()) {
            std::cerr << "File open error!" << std::endl;
        }
        else {
            clock_t start = clock();

            for (auto str : (*b)) {
                text << str << std::endl;
            }

            clock_t end = clock();

            std::cout << "Operation duration: " << (double)(end - start) / CLOCKS_PER_SEC << std::endl;
        }

        return true;
    }
    else if (action == 6) {
        std::cout << "Input filename (with .txt):" << std::endl;
        std::string filename;
        std::getline(std::cin, filename);

        std::fstream text(filename, std::ios::in);

        if (!text.is_open()) {
            std::cerr << "File open error!" << std::endl;
        }
        else {
            clock_t start = clock();

            (*b).delete_close();

            b = std::make_unique<buffer>("~temp.bin");

            std::string string_;
            while (std::getline(text, string_)) {
                (*b).push_back(string_);
            }

            clock_t end = clock();

            std::cout << "Operation duration: " << (double)(end - start) / CLOCKS_PER_SEC << std::endl;
        }
        return true;
    }

    else if (action == 7) {
        return false;
    }

    else {
        std::cout << "Unknown command. Try again!";
        return true;
    }
}

int main() {
    bool running = true;

    auto b = std::make_unique<buffer>("~temp.bin");

    while (running) {
        try {
            running = menu(b);
        }
        catch (std::runtime_error& err) {
            std::cerr << err.what() << std::endl;
        }
    }

    (*b).delete_close();
    return 0;
}
