# highloadcup2017
[![MIT License](https://img.shields.io/github/license/kswaldemar/highloadcup2017.svg?style=flat-square)](./LICENSE)

Solution for first highload mail.ru championship.

Task was to create http REST web-service, deploy it using docker and show best possible latency for each request.
Detailed info about rules and request types can be found in [official website](https://highloadcup.ru/)

## Design goals
Solution created with these ideas in mind:
 * Use modern C++17 features
 * Keep sane and good structurized code
 * Use libraries for network and json, so no invent bicycle

## Solution

Task realisation is pretty straightforward and don't use complex algorithms:
1. Get request, determine route according to documentation
2. Perform operation with database
3. Create response with result

### Project structure:
1. RequestHandler - respond for routing, extracting get parameters, passing queries to database, response formation
2. SimpleDB - database realisation, build using hashmaps. It is optimal solution, because of high performance requirements and few number of requests. So it is possible to create function for each request type. For requests with aggregation used special "index" hash maps.
3. PodTypes - just one-to-one mapping of needed primitive types. Used for json convertaion.

### Third parites:
* HTTP server built using [libwebsite](https://github.com/kswaldemar/libwebsite) - evhttp replacement for libevent
* For json interaction used [nlohmann json library](https://github.com/nlohmann/json). Pretty high level and handy. In critical places replaced with plain sprintf formatting to achieve best possible performance.

### Features

Project used C++17 standard, especially:
* filesystem - for initial database filling from json files
* optional - for passing get parameters to query function
* string_view - as lightweight replacement for string, for selecting method, uri, etc. just in message buffer from socket

## Build

### Building on local machine:

	mdkir build && cd build
    cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . 

Tested with `clang 4.0.1`

*Notice that project uses libc++ standard library instead of libstdc++*.

### Building using docker

Because championship has limits on docker container size and clang cannot fit inside need to manually copy runtime libraries into container. 
You can either install clang inside docker or 
create folder `docker-libs` and populate with necessarily libraries.

	mkdir docker-libs
    cp /lib/libc++.so.1 docker-libs
    cp /lib/libc++abi.so.1 docker-libs
    cp /lib/x86_64-linux-gnu/libgcc_s.so.1 docker-libs
    
Create docker container:

	docker build -t hlcup .
	
## License

All source code belonging to this repo distributed under [MIT License](./LICENSE).