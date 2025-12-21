FROM debian:bookworm-slim

# needed to include man pages
RUN rm /etc/dpkg/dpkg.cfg.d/docker

RUN apt-get update && apt-get install -y \
    man-db \
    manpages \
    manpages-dev \

    # ps
    procps \

    # gcc, make 
	build-essential \

	gdb \
	gdbserver \
	valgrind \
    clang-format \

    # tests 
    libcriterion-dev

CMD ["/bin/bash"]
