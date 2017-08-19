FROM debian:jessie

WORKDIR /root

RUN apt-get update \
    && apt-get install --assume-yes \
    build-essential \
    cmake \
    libboost-system1.55-dev \
    libboost-thread1.55-dev

RUN apt-get update && apt-get install -y libboost-filesystem1.55-dev
RUN apt-get install -y unzip

VOLUME /tmp

ADD build-release/web-server .
ADD server-run.sh .
ADD data local-data

EXPOSE 80

ENTRYPOINT [ "sh", "-c", "./server-run.sh" ]












