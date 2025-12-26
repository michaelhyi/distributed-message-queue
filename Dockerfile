FROM debian:bookworm-slim

# needed to include man pages
RUN rm /etc/dpkg/dpkg.cfg.d/docker

RUN apt-get update && apt-get install -y \
    less \
    vim \

    man-db \
    manpages \
    manpages-dev \

    # ps
    procps \

    # wget used for installing zookeeper
    wget \

    # zookeeper
    default-jdk \
    libzookeeper-mt-dev \

    # gcc, make 
	build-essential \

	gdb \
	gdbserver \
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
    mkdir -p /data/zookeeper && \
    echo "tickTime=2500\ndataDir=/data/zookeeper\nclientPort=2181\nmaxClientCnxns=80" > /opt/zookeeper/conf/zoo.cfg

# start & init zookeeper server, start shell
CMD /opt/zookeeper/bin/zkServer.sh start && \
    chmod +x ./scripts/init_zookeeper.sh && \
    ./scripts/init_zookeeper.sh && \
    /bin/bash
