#include "tcp_sponge_socket.hh"
#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>
#include <random>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).
    // uint16_t portnumber = ((std::random_device()()) % 50000) + 1025;

    CS144TCPSocket socket1;
    // socket1.bind(Address("127.0.0.1", portnumber));
    socket1.connect(Address(host, "http"));
    socket1.write("GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n\r\n");
    socket1.shutdown(SHUT_WR);
    /*
        When the shutdown(SHUT_WR) are called the server will send you one reply and then will end its own
        outgoing bytestream (the one from the server’s socket to your socket). If you don’t shut down your 
        outgoing byte stream, the server will wait around for a while for you to send additional requests and 
        won’t end its outgoing byte stream either.
    */
    while(!socket1.eof())
        std::cout << socket1.read();
    /*
        Make sure to read and print all the output from the server until the socket reaches “EOF” (end of file)
        --- a single call to read is not enough.

        Example (?)
    */
    // socket1.close();

    // cerr << "Function called: get_URL(" << host << ", " << path << ").\n";
    // cerr << "Warning: get_URL() has not been implemented yet.\n";
    socket1.wait_until_closed();
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
