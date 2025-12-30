FROM debian:bookworm-slim

# needed to include man pages
RUN rm /etc/dpkg/dpkg.cfg.d/docker

RUN apt-get update && apt-get install -y \
    less \
    vim \
    # ps
    procps \
    wget \

    man-db \
    manpages \
    manpages-dev \

    # zookeeper
    default-jdk \
    libzookeeper-mt-dev \

    # gcc, make 
	build-essential \
	gdb \
	valgrind \
    clang-format \

    # tests
    libcriterion-dev && \

    rm -rf /var/lib/apt/lists/*

# install & configure zookeeper
RUN cd /opt && \
    wget https://dlcdn.apache.org/zookeeper/zookeeper-3.8.5/apache-zookeeper-3.8.5-bin.tar.gz && \
    tar -xvzf apache-zookeeper-3.8.5-bin.tar.gz && \
    mv apache-zookeeper-3.8.5-bin zookeeper && \
    rm apache-zookeeper-3.8.5-bin.tar.gz && \
    mkdir -p /data/zookeeper/dev && \
    mkdir -p /data/zookeeper/test

# start & init zookeeper servers, start shell
CMD /opt/zookeeper/bin/zkServer.sh start ./conf/zoo_dev.cfg && \
    chmod +x ./scripts/init_zookeeper.sh && \
    ./scripts/init_zookeeper.sh 127.0.0.1:2181 && \

    /opt/zookeeper/bin/zkServer.sh start ./conf/zoo_test.cfg && \

    /bin/bash
