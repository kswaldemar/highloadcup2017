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

VOLUME /tmp

ADD build-release/web-server .
ADD server-run.sh .
ADD data local-data

ADD build-release/3rdparty/website/libwebsite.so /lib
ADD docker-libs /lib

EXPOSE 80

ENTRYPOINT [ "sh", "-c", "./server-run.sh" ]












