#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/process.hpp>
#include <boost/regex.hpp>
#include "Messenger.h"
#include "Logger.h"

namespace asio = boost::asio;
namespace bp = boost::process;
using asio::ip::tcp;
using callback_type = std::function<void(const boost::system::error_code&, std::size_t size)>;

int main(int argc, char *argv[]) {
    try {
        // Accept a connection to use this builder
        asio::io_service io_service;
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8080));
        tcp::socket socket(io_service);
        acceptor.accept(socket);

        logger::write("Received connection from: " + boost::lexical_cast<std::string>(socket.remote_endpoint()));

        // Read the recipe file
        Messenger client_messenger(socket);
        client_messenger.receive_file("container.def");

        logger::write("Recipe file received");

        // Create a pipe to communicate with our build subprocess
        bp::async_pipe std_pipe(io_service);

        // Launch our build as a subprocess
        // We use "unbuffer" to fake the build into thinking it has a real TTY, which the command output eventually will
        // This causes things like wget and color ls to work nicely
        // TODO : perhaps set TERM as well to something like xterm ?
        std::string build_command("/usr/bin/sudo /usr/bin/unbuffer /usr/local/bin/singularity build ./container.img ./container.def");

        bp::group group;
        std::error_code build_ec;
        bp::child build_child(build_command, bp::std_in.close(), (bp::std_out & bp::std_err) > std_pipe, group, build_ec);
        if(build_ec) {
            logger::write("subprocess error: " + build_ec.message());
        }

        logger::write("Running build command: " + build_command);

        // Read process pipe output and write it to the client
        // line buffer(ish) by reading from the pipe until we hit \n, \r
        // NOTE: read_until will fill buffer until line_matcher is satisfied but generally will contain additional data.
        // This is fine as all we care about is dumping everything from std_pipe to our buffer and don't require exact line buffering
        // TODO: just call read_some perhaps?
        asio::streambuf buffer;
        boost::regex line_matcher{"\\r|\\n"};

        // Callback for handling reading from pipe and sending output to client
        callback_type read_std_pipe = [&](const boost::system::error_code& ec, std::size_t size) {
            client_messenger.send(buffer);

            if(size > 0 && !ec) {
                asio::async_read_until(std_pipe, buffer, line_matcher, read_std_pipe);
            }
            else if(size == 0 && ec == asio::error::eof) {
                logger::write("build output EOF");
            }
            else {
                throw std::system_error(EBADMSG, std::generic_category(), "Error reading build output" + ec.message());
            }
        };
        // Start reading child stdout/err from pipe
        asio::async_read_until(std_pipe, buffer, line_matcher, read_std_pipe);

        io_service.run();

        // Get the return value from the build subprocess
        build_child.wait();
        int build_code = build_child.exit_code();

        logger::write("Sending image to client");

        // Send the container to the client
        if(build_code == 0) {
            logger::write("Image failed to build");
            client_messenger.send_file("container.img");
        }

        logger::write("Finished sending image to client");
    }
    catch (std::exception &e) {
        logger::write(std::string("Build error: ") + e.what());
    }

    return 0;
}