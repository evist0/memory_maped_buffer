#ifndef LAB_buffer_HPP
#define LAB_buffer_HPP

#include <fstream>
#include <string>

class buffer_proxy;

class buffer_iterator;

class buffer : private std::fstream {
    public:
        explicit buffer(const std::string& filename, size_t capacity = 4);

        size_t capacity();
        size_t size();
        buffer_proxy operator[](const size_t& index);

        void push_back(const std::string& value);
        void insert(const size_t& index, const std::string& value);
        void erase(const size_t& index);

        buffer_iterator begin();
        buffer_iterator end();

        void delete_close();

        friend class buffer_proxy;

        friend class buffer_iterator;

    private:
        std::string filename_;

        void realloc();

        void set_size(const size_t& new_size);

        void set_offset(const size_t& index, const size_t& new_offset);
};

class buffer_proxy {
    public:
        operator std::string(); // NOLINT(google-explicit-constructor)
        bool operator==(const buffer_proxy& rhs);
        bool operator!=(const buffer_proxy& rhs);
        buffer_proxy& operator=(const std::string& rhs);
        friend std::ostream& operator<<(std::ostream& out, const buffer_proxy& buffer);

    private:
        buffer_proxy(buffer& buffer, const size_t& index);

        buffer& buffer_;
        size_t index_;

        friend class buffer_iterator;
};

std::ostream& operator<<(std::ostream& out, const buffer_proxy& buffer);

class buffer_iterator {
    public:
        buffer_iterator& operator++();
        const buffer_iterator operator++(int);

        buffer_iterator& operator--();
        const buffer_iterator operator--(int);

        bool operator==(const buffer_iterator& rhs);
        bool operator!=(const buffer_iterator& rhs);

        buffer_proxy operator*();

    private:
        buffer_iterator(buffer& buffer, const size_t& index);

        buffer& buffer_;
        size_t index_;

        friend class buffer;
};

#endif //LAB_buffer_HPP
