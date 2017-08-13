FROM debian:jessie

WORKDIR /root

RUN apt-get update \
    && apt-get install --assume-yes \
    build-essential \
    cmake \
    libboost-system1.55-dev \
    libboost-thread1.55-dev

ADD 3rdparty 3rdparty
ADD src src
ADD CMakeLists.txt CMakeLists.txt

RUN mkdir -p build && cd build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && make

EXPOSE 80

CMD ./build/web-server












