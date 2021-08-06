/*
Name: John Yu
ID #: 1615007
CMPUT 275, Winter 2021

Assign 2: Client/Server application
*/

#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <list>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "wdigraph.h"
#include "dijkstra.h"

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the point that is closest to a given point, pt
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// reads graph description from the input file and builts a graph instance
void readGraph(const string& filename, WDigraph& g,
	unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // starting a new string
        ++at;
      } else {
        // appending a character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // adding a new vertex
      int id = stoi(p[1]);
      assert(id == stoll(p[1]));  // sanity check: asserts
      // if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    } else {
      // adding a new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}

// Keep in mind that in Part I, your program must handle 1 request
// but in Part 2 you must serve the next request as soon as you are
// done handling the previous one
int main(int argc, char* argv[]) {
  WDigraph graph;
  unordered_map<int, Point> points;

  // build the graph
  readGraph("./server/edmonton-roads-2.0.1.txt", graph, points);

  // In Part 2, client and server communicate using a pair of sockets
  // modify the part below so that the route request is read from a socket
  // (instead of stdin) and the route information is written to a socket
  int port = atoi(argv[1]);
  struct sockaddr_in my_addr, peer_addr;
  memset(&my_addr, '\0', sizeof my_addr);
  int lstn_socket_desc, conn_socket_desc;

  lstn_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (lstn_socket_desc == -1) {
  	cerr << "Listening socket creation failed!\n";
  	return 1;
  }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(lstn_socket_desc, (struct sockaddr *)
  	&my_addr, sizeof my_addr) == -1) {
  	cerr << "Binding failed!\n";
  	close(lstn_socket_desc);
  	return 1;
  }
  cout << "Binding was successful\n";

  if (listen(lstn_socket_desc, 50) == -1) {
  	cerr << "Cannot listen to the specified socket!\n";
  	close(lstn_socket_desc);
  	return 1;
  }

  socklen_t peer_addr_size = sizeof my_addr;

  string output = "Hello World";

	conn_socket_desc = accept(lstn_socket_desc, (struct sockaddr * )
		&peer_addr, &peer_addr_size);
	if (conn_socket_desc == -1) {
		cerr << "Connection socket creation failed!\n";
		return 1;
	}
	cout << "Connection request accepted from " << inet_ntoa(
		peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

	// read a request
	char c[1024];
	Point sPoint, ePoint;
  while (true) {
  	// Start Point
  	int rec_size = recv(conn_socket_desc, c, 1024, 0);
  	if (rec_size == -1) {
  		cout << "Timeout occured... still waiting!\n";
  		continue;
  	}

  	if (strcmp(c, "Q") == 0) {  // Client tells server to quit
  		break;
  	}

  	string coordinates(c);
  	int delim0, delim1, delim2;
    delim0 = coordinates.find(" ", 2);  // Position of first space
    delim1 = coordinates.find(" ", delim0 + 1);  // Position of second space
    delim2 = coordinates.find(" ", delim1 + 1);  // Position of third space
    sPoint.lat = static_cast<long long>(stod(coordinates.substr(
    	2, delim0 - 1)));
    sPoint.lon = static_cast<long long>(stod(coordinates.substr(
    	delim0 + 1, delim1 - delim0 - 1)));
    ePoint.lat = static_cast<long long>(stod(coordinates.substr(
    	delim1 + 1, delim2 - delim1 - 1)));
    ePoint.lon = static_cast<long long>(stod(coordinates.substr(
    	delim2 + 1, coordinates.size() - delim2 - 1)));

  	// c is guaranteed to be 'R' in part 1, no need to error check until part 2

  	// get the points closest to the two points we read

  	int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

  	// run dijkstra's, this is the unoptimized version that does not stop
  	// when the end is reached but it is still fast enough
  	unordered_map<int, PIL> tree;
  	dijkstra(graph, start, tree);

  	// no path
  	if (tree.find(end) == tree.end()) {
  		output = "N 0";
    	send(conn_socket_desc, output.c_str(), output.length() + 1, 0);
    	rec_size = recv(conn_socket_desc, c, 1024, 0);  // Receive input "A"
    	output = "E";
    	send(conn_socket_desc, output.c_str(), output.length() + 1, 0);
  	} else {
    	// read off the path by stepping back through the search tree
    	list<int> path;
    	while (end != start) {
      		path.push_front(end);
      		end = tree[end].first;
    	}
    	path.push_front(start);

    	// output the path
    	// output = "N " + path.size();
    	output = "N " + to_string(path.size());
    	send(conn_socket_desc, output.c_str(), output.length() + 1, 0);
    	for (int v : path) {
    		rec_size = recv(conn_socket_desc, c, 1024, 0);  // Receive the input "A"
      		output = "W " + to_string(points[v].lat) + " " + to_string(
      			points[v].lon);
      		send(conn_socket_desc, output.c_str(), output.length() + 1, 0);
    	}
    	rec_size = recv(conn_socket_desc, c, 1024, 0);  // Receive the input "A"
    	output = "E";
    	send(conn_socket_desc, output.c_str(), output.length() + 1, 0);
  	}
  }

  close(conn_socket_desc);

  return 0;
}
