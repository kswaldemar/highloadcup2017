FROM debian:jessie

WORKDIR /root

RUN apt-get update \
    && apt-get install --assume-yes \
    build-essential \
    cmake \
    libboost-system1.55-dev \
    libboost-thread1.55-dev

VOLUME /tmp

ADD build-release/web-server web-server

EXPOSE 80

ENTRYPOINT [ "sh", "-c", "./web-server" ]












