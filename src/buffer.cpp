#include "../include/buffer.hpp"
#include <array>
#include <stdexcept>
#include <cassert>
#include <cmath>

constexpr static std::array<char, 4> magic{ 0x01, 0x02, 0x03, 0x04 };

constexpr static size_t capacity_pos = magic.size();

constexpr static size_t size_pos = capacity_pos + sizeof(size_t);

constexpr static size_t offsets_pos = size_pos + sizeof(size_t);

constexpr static std::ios::openmode mode = std::ios::binary | std::ios::in | std::ios::out;

buffer::buffer(const std::string& filename, size_t capacity)
        :std::fstream(filename, mode) {
    filename_ = filename;

    if (!std::fstream::is_open()) {
        std::fstream::open(filename, mode | std::ios_base::trunc);
    }

    std::fstream::exceptions(std::ios::badbit | std::ios::failbit);

    std::array<char, 4> temp{};
    auto read = std::fstream::readsome(temp.data(), magic.size());
    if (read != magic.size() || temp != magic) {
        std::fstream::close();
        std::fstream::open(filename, mode | std::ios::trunc);
        std::fstream::write(magic.data(), magic.size());

        size_t capacity_ = capacity;
        std::fstream::write(reinterpret_cast<const char*>(&capacity), sizeof(capacity));

        size_t size_ = 0;
        std::fstream::write(reinterpret_cast<const char*>(&size_), sizeof(size_));

        size_t offset_ = 0;
        for (auto i = 0; i < capacity_; i++) {
            std::fstream::write(reinterpret_cast<const char*>(&offset_), sizeof(offset_));
        }
    }
}

size_t buffer::capacity() {
    std::fstream::seekg(capacity_pos, std::ios::beg);

    size_t v;
    std::fstream::read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

void buffer::set_size(const size_t& new_size) {
    std::fstream::seekp(size_pos, std::ios::beg);

    std::fstream::write(reinterpret_cast<const char*>(&new_size), sizeof(new_size));
}

size_t buffer::size() {
    std::fstream::seekg(size_pos, std::ios::beg);

    size_t v;
    std::fstream::read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

void buffer::set_offset(const size_t& index, const size_t& new_offset) {
    if (index > size()) {
        throw std::runtime_error("Out of bounds");
    }

    std::fstream::seekp(offsets_pos + index * sizeof(size_t), std::ios::beg);

    size_t new_offset_ = new_offset;
    std::fstream::write(reinterpret_cast<const char*>(&new_offset_), sizeof(new_offset_));
}

buffer_proxy buffer::operator[](const size_t& index) {
    if (index > size()) {
        throw std::runtime_error("Out of bounds");
    }

    return *buffer_iterator(*this, index);
}

void buffer::realloc() {
    auto capacity_ = capacity();
    auto size_ = size();

    buffer temp_("~temp", capacity_ * 2);
    temp_.set_size(size_);

    for (size_t i = 0; i < size_; i++) {
        std::fstream::seekg(offsets_pos + i * sizeof(size_t), std::ios::beg);

        size_t offset = 0;
        std::fstream::read(reinterpret_cast<char*>(&offset), sizeof(size_t));

        std::string value = *buffer_iterator{ *this, i };

        temp_.insert(i, value);
    }

    temp_.close();
    std::fstream::close();

    std::remove(filename_.c_str());
    std::rename("~temp", filename_.c_str());

    std::fstream::open(filename_, mode);
}

void buffer::push_back(const std::string& value) {
    auto capacity_ = capacity();
    auto size_ = size();

    if (size_ == capacity_) {
        realloc();
    }

    std::fstream::seekp(0, std::ios::end);
    auto offset = tellp();

    auto length_ = value.length();
    std::fstream::write(reinterpret_cast<const char*>(&length_), sizeof(size_t));

    auto string_ = value.data();
    std::fstream::write(string_, length_);

    std::fstream::seekp(offsets_pos + size_ * sizeof(size_t), std::ios::beg);
    std::fstream::write(reinterpret_cast<const char*>(&offset), sizeof(size_t));

    set_size(size_ + 1);
}

void buffer::insert(const size_t& index, const std::string& value) {
    auto capacity_ = capacity();
    auto size_ = size();

    if (index > size_) {
        throw std::runtime_error("Out of bounds");
    }

    if (size_ == capacity_) {
        realloc();
    }

    std::fstream::seekp(0, std::ios::end);
    auto offset = tellp();

    auto length_ = value.length();
    std::fstream::write(reinterpret_cast<const char*>(&length_), sizeof(size_t));

    auto string_ = value.data();
    std::fstream::write(string_, length_);

    for (auto i = size_; i > index; i--) {
        size_t bubble;
        std::fstream::seekg(offsets_pos + (i - 1) * sizeof(size_t), std::ios::beg);
        std::fstream::read(reinterpret_cast<char*>(&bubble), sizeof(bubble));

        std::fstream::seekp(offsets_pos + i * sizeof(size_t), std::ios::beg);
        std::fstream::write(reinterpret_cast<const char*>(&bubble), sizeof(bubble));
    }

    std::fstream::seekp(offsets_pos + index * sizeof(size_t), std::ios::beg);
    std::fstream::write(reinterpret_cast<const char*>(&offset), sizeof(size_t));

    set_size(size_ + 1);
}

void buffer::erase(const size_t& index) {
    size_t size_ = size();

    // Нашли строку для удаления
    seekg(offsets_pos + index * sizeof(size_t), std::ios::beg);

    size_t removed_string_offset;
    std::fstream::read(reinterpret_cast<char*>(&removed_string_offset), sizeof(removed_string_offset));


    // Нашли следующую строку
    seekg(removed_string_offset, std::ios::beg);

    size_t length_;
    std::fstream::read(reinterpret_cast<char*>(&length_), sizeof(length_));

    size_t next_string_offset = removed_string_offset + sizeof(size_t) + length_;

    // Нашли конец файла
    seekg(0, std::ios::end);
    size_t end = tellg();

    // Удалили в офсетах
    for (auto i = index; i < size_; i++) {
        size_t bubble;
        std::fstream::seekg(offsets_pos + (i + 1) * sizeof(size_t), std::ios::beg);
        std::fstream::read(reinterpret_cast<char*>(&bubble), sizeof(bubble));

        std::fstream::seekp(offsets_pos + i * sizeof(size_t), std::ios::beg);
        std::fstream::write(reinterpret_cast<const char*>(&bubble), sizeof(bubble));
    }
    set_offset(size_ - 1, 0);

    // Поджали оффсеты строк
    for (size_t i = 0; i < size_; i++) {
        size_t bubble;

        seekg(offsets_pos + i * sizeof(size_t), std::ios::beg);
        read(reinterpret_cast<char*>(&bubble), sizeof(size_t));

        if (bubble >= removed_string_offset) {
            bubble -= sizeof(size_t) + length_;

            seekp(offsets_pos + i * sizeof(size_t), std::ios::beg);
            write(reinterpret_cast<const char*>(&bubble), sizeof(size_t));
        }
    }

    auto shift = end - next_string_offset;

    // Поджали строки
    for (size_t i = 0; i < shift; i++) {
        char bubble;
        seekg(next_string_offset + i, std::ios::beg);
        read(reinterpret_cast<char*>(&bubble), sizeof(char));

        seekp(removed_string_offset + i, std::ios::beg);
        write(reinterpret_cast<const char*>(&bubble), sizeof(char));
    }

    set_size(size_ - 1);
}

void buffer::delete_close() {
    std::fstream::close();

    remove(filename_.c_str());
}

buffer_proxy::buffer_proxy(buffer& buffer, const size_t& index)
        :buffer_(buffer), index_(index) {
}

buffer_proxy::operator std::string() {
    buffer_.seekg(offsets_pos + sizeof(size_t) * index_, std::ios::beg);

    size_t offset;
    buffer_.read(reinterpret_cast<char*>(&offset), sizeof(size_t));

    buffer_.seekg(offset, std::ios::beg);

    size_t size;
    buffer_.read(reinterpret_cast<char*>(&size), sizeof(size_t));

    char* str = new char[size + 1];

    buffer_.read(str, size);
    str[size] = '\0';

    std::string value = std::string(str);

    delete[] str;

    return value;
}

bool buffer_proxy::operator==(const buffer_proxy& rhs) {
    assert(&buffer_ == &rhs.buffer_);

    return this->index_ == rhs.index_;
}

bool buffer_proxy::operator!=(const buffer_proxy& rhs) {
    return !this->operator==(rhs);
}

std::ostream& operator<<(std::ostream& out, const buffer_proxy& buffer) {
    out << static_cast<std::string>(const_cast<buffer_proxy&>(buffer));
    return out;
}

buffer_proxy& buffer_proxy::operator=(const std::string& rhs) {
    if (static_cast<std::string>(const_cast<buffer_proxy&>(*this)) == rhs) {
        return *this;
    }

    size_t size_ = buffer_.size();

    buffer_.seekg(offsets_pos + index_ * sizeof(size_t));
    size_t offset;
    buffer_.read(reinterpret_cast<char*>(&offset), sizeof(size_t));

    size_t length_ = (this->operator std::string()).length();
    ptrdiff_t delta = rhs.length() - length_;

    size_t begin = offset + sizeof(size_t) + length_;

    buffer_.seekp(0, std::ios::end);
    size_t end = buffer_.tellp();

    if (delta > 0) {
        buffer_.seekp(0, std::ios::end);

        for (size_t i = 0; i < delta; i++) {
            char bubble = '\0';
            buffer_.write(&bubble, 1);
        }

        end += delta;

        size_t shift = end - begin;

        begin = end - delta;

        for (size_t i = 0; i < shift; i++) {
            char bubble;
            buffer_.seekg(begin - i - 1, std::ios::beg);
            buffer_.read(reinterpret_cast<char*>(&bubble), sizeof(char));

            buffer_.seekp(end - i - 1, std::ios::beg);
            buffer_.write(reinterpret_cast<const char*>(&bubble), sizeof(char));
        }

    }
    else {

        size_t shift = end - begin;

        begin += delta;
        end = begin - delta;

        for (size_t i = 0; i < shift; i++) {
            char bubble;
            buffer_.seekg(end + i, std::ios::beg);
            buffer_.read(reinterpret_cast<char*>(&bubble), sizeof(char));

            buffer_.seekp(begin + i, std::ios::beg);
            buffer_.write(reinterpret_cast<const char*>(&bubble), sizeof(char));
        }
    }

    // Поджали оффсеты строк
    for (size_t i = 0; i < size_; i++) {
        size_t bubble;

        buffer_.seekg(offsets_pos + i * sizeof(size_t), std::ios::beg);
        buffer_.read(reinterpret_cast<char*>(&bubble), sizeof(size_t));

        if (bubble > offset) {
            bubble += delta;

            buffer_.seekp(offsets_pos + i * sizeof(size_t), std::ios::beg);
            buffer_.write(reinterpret_cast<const char*>(&bubble), sizeof(size_t));
        }
    }

    buffer_.seekp(offset, std::ios::beg);

    length_ += delta;
    buffer_.write(reinterpret_cast<const char*>(&length_), sizeof(size_t));
    buffer_.write(rhs.data(), length_);

    return *this;
}

buffer_iterator::buffer_iterator(buffer& buffer, const size_t& index)
        :buffer_(buffer), index_(index) {
}

buffer_iterator& buffer_iterator::operator++() {
    ++index_;
    return *this;
}

const buffer_iterator buffer_iterator::operator++(int) {
    buffer_iterator prev{ buffer_, index_ };
    ++*this;
    return prev;
}

buffer_iterator& buffer_iterator::operator--() {
    --index_;
    return *this;
}

const buffer_iterator buffer_iterator::operator--(int) {
    buffer_iterator prev{ buffer_, index_ };
    --*this;
    return prev;
}

bool buffer_iterator::operator==(const buffer_iterator& rhs) {
    assert(&buffer_ == &rhs.buffer_);

    return index_ == rhs.index_;
}

bool buffer_iterator::operator!=(const buffer_iterator& rhs) {
    return !(*this == rhs);
}

buffer_proxy buffer_iterator::operator*() {
    return { buffer_, index_ };
}

buffer_iterator buffer::begin() {
    return { *this, 0 };
}

buffer_iterator buffer::end() {
    std::fstream::seekg(size_pos, std::ios::beg);

    size_t size;
    std::fstream::read(reinterpret_cast<char*>(&size), sizeof(size));

    return { *this, size };
}