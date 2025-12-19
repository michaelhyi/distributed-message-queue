FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    procps \
	build-essential \
	gdb \
	gdbserver \
	valgrind \
    clang-format \
    libcriterion-dev

CMD ["/bin/bash"]
