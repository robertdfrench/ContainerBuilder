#include "Messenger.h"
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <iostream>
#include <fstream>

// TODO: move this into .cpp as it's not templated!

// Send the header, which is the size of the message that will immediately follow
void Messenger::send_header(header_t message_size) {
    auto header = asio::buffer(&message_size, header_size());
    asio::write(socket, header, asio::transfer_exactly(header_size()));
}

// Send the header asynchrnously, which is the size of the message that will immediately follow
void Messenger::async_send_header(header_t message_size, asio::yield_context yield) {
    auto header = asio::buffer(&message_size, header_size());
    asio::async_write(socket, header, asio::transfer_exactly( header_size()), yield);
}

// Receive the message header
header_t Messenger::receive_header() {
    header_t message_size;
    auto header = asio::buffer(&message_size, header_size());
    asio::read(socket, header, asio::transfer_exactly(header_size()));
    return message_size;
}

// Receive the message header
header_t Messenger::async_receive_header(asio::yield_context yield) {
    header_t message_size;
    auto header = asio::buffer(&message_size, header_size());
    asio::async_read(socket, header, asio::transfer_exactly(header_size()), yield);
    return message_size;
}

// Send a string message
void Messenger::send(const std::string& message) {
    auto message_size = message.size();

    send_header(message_size);

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::write(socket, body, asio::transfer_exactly(message_size));
}

// Send a string message asynchronously
void Messenger::async_send(const std::string& message, asio::yield_context yield) {
    auto message_size = message.size();

    async_send_header(message_size, yield);

    // Write the message body
    auto body = asio::buffer(message.data(), message_size);
    asio::async_write(socket, body, asio::transfer_exactly(message_size), yield);
}

// Send a streambuf message asynchronously
void Messenger::async_send(asio::streambuf& message_body, asio::yield_context yield) {
    auto message_size = message_body.size();

    async_send_header(message_size, yield);

    // Write the message body
    asio::async_write(socket, message_body, asio::transfer_exactly(message_size), yield);
}

// Send a file as a message
void Messenger::send_file(boost::filesystem::path file_path, std::size_t chunk_size) {
    std::ifstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    auto file_size = boost::filesystem::file_size(file_path);

    // Send message header
    send_header(file_size);

    // Send file as message body, in chunk_size sections
    auto bytes_remaining = file_size;
    std::vector<char> buffer_storage;
    buffer_storage.reserve(chunk_size);
    auto buffer = asio::buffer(buffer_storage);
    do {
        auto bytes_to_send = std::min(bytes_remaining, chunk_size);

        file.read(buffer_storage.data(), bytes_to_send);

        auto bytes_sent = asio::write(socket, buffer, asio::transfer_exactly(bytes_to_send));

        bytes_remaining -= bytes_sent;
    } while(bytes_remaining);
}

// Send a file as a message asynchronously
void Messenger::async_send_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size) {
    std::ifstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Open file and get size
    file.open(file_path.string(), std::fstream::in | std::fstream::binary);
    auto file_size = boost::filesystem::file_size(file_path);

    async_send_header(file_size, yield);

    // Send file in chunk_size bits
    auto bytes_remaining = file_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);
    do {
        auto bytes_to_send = std::min(bytes_remaining, chunk_size);

        file.read(buffer_storage.data(), bytes_to_send);

        auto bytes_sent = asio::async_write(socket, buffer, asio::transfer_exactly(bytes_to_send), yield);

        bytes_remaining -= bytes_sent;
    } while(bytes_remaining);
}

// Receive a string message
std::string Messenger::receive() {
    // Read message size from header
    auto message_size = receive_header();

    // Read the message body
    asio::streambuf buffer;
    asio::read(socket, buffer, asio::transfer_exactly(message_size));

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                      std::istreambuf_iterator<char>());

    return body;
}

// Receive a string message asynchronously
std::string Messenger::async_receive(asio::yield_context yield) {
    // Read message size from header
    auto message_size = async_receive_header(yield);

    // Read the message body
    asio::streambuf buffer;
    asio::async_read(socket, buffer, asio::transfer_exactly(message_size), yield);

    // Construct a string from the message body
    std::string body((std::istreambuf_iterator<char>(&buffer)),
                      std::istreambuf_iterator<char>());

    return body;
}

void Messenger::receive_file(boost::filesystem::path file_path, std::size_t chunk_size) {
    std::ofstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);

    auto total_size = receive_header();

    // Receive file in chunk_size messages
    auto bytes_remaining = total_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);

    do {
        auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

        auto bytes_received = asio::read(socket, buffer, asio::transfer_exactly(bytes_to_receive));

        file.write(buffer_storage.data(), bytes_received);

        bytes_remaining -= bytes_received;

    } while(bytes_remaining);
}

void Messenger::async_receive_file(boost::filesystem::path file_path, asio::yield_context yield, std::size_t chunk_size) {
    std::ofstream file;

    // Throw exception if we run into any file issues
    file.exceptions(std::fstream::failbit | std::ifstream::badbit);

    // Openfile for writing
    file.open(file_path.string(), std::fstream::out | std::fstream::binary | std::fstream::trunc);

    auto total_size = async_receive_header(yield);

    // Receive file in chunk_size messages
    auto bytes_remaining = total_size;
    std::vector<char> buffer_storage(chunk_size);
    auto buffer = asio::buffer(buffer_storage);

    do {
        auto bytes_to_receive = std::min(bytes_remaining, chunk_size);

        auto bytes_received = asio::async_read(socket, buffer, asio::transfer_exactly(bytes_to_receive), yield);

        file.write(buffer_storage.data(), bytes_received);

        bytes_remaining -= bytes_received;

    } while(bytes_remaining);
}
