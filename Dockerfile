FROM debian:jessie

WORKDIR /root

RUN apt-get update \
    && apt-get install --assume-yes \
    build-essential \
    cmake \
    libboost-system1.55-dev \
    libboost-thread1.55-dev

RUN apt-get update && apt-get install -y \
    unzip libssl-dev libev-dev

RUN rm -rf /var/lib/apt/lists/*

VOLUME /tmp

ADD docker-libs /lib

ADD build-release/3rdparty/libwebsite/libwebsite.so /lib
ADD build-release/server-run.sh .
ADD build-release/web-server .

EXPOSE 80

ENTRYPOINT [ "sh", "-c", "./server-run.sh" ]












