// working static
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <server hostname> <port>" << std::endl;
        return 1;
    }

    std::string server_hostname = argv[1];
    std::string port = argv[2];

    // Build the command to execute the C client program with the provided arguments
    std::string cmd = "./ipkcpc -h " + server_hostname + " -p " + port + " -m tcp";

    // Open a pipe to the C client program
    FILE *pipe = popen(cmd.c_str(), "w");
    if (!pipe) {
        std::cerr << "Failed to open pipe to C client program" << std::endl;
        return 1;
    }

    // Read input from the user and send it to the server
    while (true) {
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        // Send the user's input to the server via the pipe
        fprintf(pipe, "%s\n", input.c_str());
        fflush(pipe);

        // Read the server's response from the pipe and print it
        char response[1024];
        fgets(response, sizeof(response), pipe);
        std::cout << response;
    }

    // Close the pipe to the C client program
    pclose(pipe);

    return 0;
}
