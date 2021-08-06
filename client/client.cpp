/*
Name: John Yu
ID #: 1615007
CMPUT 275, Winter 2021

Assign 2: Client/Server application
*/

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
// Add more libraries, macros, functions, and global variables if needed

using namespace std;

int create_and_open_fifo(const char * pname, int mode) {
    // creating a fifo special file in the current working directory
    // with read-write permissions for communication with the plotter
    // both proecsses must open the fifo before they can perform
    // read and write operations on it
    if (mkfifo(pname, 0666) == -1) {
        cout << "Unable to make a fifo. ";
        cout << "Ensure that this pipe does not exist already!" << endl;
        exit(-1);
    }

    // opening the fifo for read-only or write-only access
    // a file descriptor that refers to the open file description is
    // returned
    int fd = open(pname, mode);

    if (fd == -1) {
        cout << "Error: failed on opening named pipe." << endl;
        exit(-1);
    }

    return fd;
}

int main(int argc, char const *argv[]) {
    const char *inpipe = "inpipe";
    const char *outpipe = "outpipe";

    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

    // Your code starts here

    // Here is what you need to do:
    // 1. Establish a connection with the server
    int port = atoi(argv[1]);
    struct sockaddr_in my_addr, peer_addr;
    memset(&my_addr, '\0', sizeof my_addr);
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        cerr << "Listening socket creation failed!\n";
        return 1;
    }
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    peer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(socket_desc, (struct sockaddr *)
        &peer_addr, sizeof peer_addr) == -1) {
        cerr << "Cannot connect to the host!\n";
        close(socket_desc);
        return 1;
    }
    cout << "Connection established with " << inet_ntoa(
        peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

    char c[1024];  // Gets input from inpipe
    char inbound[1024];  // Gets input from socket
    int s;  // Stores the number of bytes of input from inpipe or socket
    int num;  // Number of waypoints. Ex: If input is N 8 num would be 8
    string outbound;  // String to output to outpipe
    string output;  // String to output to socket
    while (true) {
        // 2. Read coordinates of start and end points
        // from inpipe (blocks until they are selected)
        // Start point
        s = read(in, c, 1024);
        string start(c);
        // Destination point
        s = read(in, c, 1024);
        string des(c);

        // If 'Q' is read instead of the coordinates then go to Step 7
        if (strcmp(c, "Q") == 0) {
            output = "Q";
            send(socket_desc, output.c_str(), output.length() + 1, 0);
            break;
        }
        // 3. Write to the socket
        output = start.substr(0, start.size() - 1) + " " + des.substr(
            0, des.size() - 1);
        int delim0, delim1, delim2;  // Stores index position of space
        delim0 = output.find(" ");  // First space
        delim1 = output.find(" ", delim0 + 1);  // Second space
        delim2 = output.find(" ", delim1 + 1);  // Third space
        double sx0, sy0, ex0, ey0;  // Start and end coordinates as double
        long long sx1, sy1, ex1, ey1;  // Start and end coordinates as long long
        sx0 = stod(output.substr(0, delim0 - 1));
        sy0 = stod(output.substr(delim0 + 1, delim1 - delim0 - 1));
        ex0 = stod(output.substr(delim1 + 1, delim2 - delim1 - 1));
        ey0 = stod(output.substr(delim2 + 1, output.size() - delim2 - 1));
        sx1 = static_cast<long long>(sx0 * 100000);
        sy1 = static_cast<long long>(sy0 * 100000);
        ex1 = static_cast<long long>(ex0 * 100000);
        ey1 = static_cast<long long>(ey0 * 100000);
        output = "R " + to_string(sx1) + " " + to_string(
            sy1) + " " + to_string(ex1) + " " + to_string(ey1);
        send(socket_desc, output.c_str(), output.length() + 1, 0);

        // 4. Read coordinates of waypoints one at a time
        // (blocks until server writes them)
        s = recv(socket_desc, inbound, 1024, 0);  // Receive number
        // of waypoints from server
        string numPoints(inbound);
        num = stoi(numPoints.substr(2, numPoints.size() - 1 - 1));
        num += 1;  // Also need to get "E" from server
        output = "A";
        for (int i = 0; i < num; i++) {
            // Send "A" to server
            send(socket_desc, output.c_str(), output.length() + 1, 0);
            s = recv(socket_desc, inbound, 1024, 0);  // Receive waypoint
            if (strcmp(inbound, "E") == 0) {
                outbound = "E\n";
                if (write(out, outbound.c_str(), outbound.length()) == -1) {
                    cerr << "Error: Write operation failed" << endl;
                }
                break;
            }

            // 5. Write these coordinates to outpipe
            string waypoint(inbound);
            delim0 = 1;  // First character is W
            delim1 = waypoint.find(" ", delim0 + 1);  // Second space
            // separates x and y coordinates
            long long xll, yll;  // long long versions of the coordinates
            double xd, yd;  // double versions of the coordinates
            string xs, ys;  // string versions of the coordinates
            xll = stoll(waypoint.substr(delim0 + 1, delim1 - delim0 - 1));
            yll = stoll(waypoint.substr(
                delim1 + 1, waypoint.size() - delim1 - 1));
            xd = xll / 100000.0;
            yd = yll / 100000.0;
            xs = to_string(xd);
            ys = to_string(yd);
            outbound = xs + " " + ys + "\n";
            if (write(out, outbound.c_str(), outbound.length()) == -1) {
                cerr << "Error: Write operation failed" << endl;
            }
        }
        // 6. Go to Step 2
    }

    // 7. Close the socket and pipes
    close(socket_desc);

    // Your code ends here

    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
