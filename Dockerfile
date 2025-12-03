FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
	build-essential \
	gdb \
	valgrind \
    clang-format \
    libcriterion-dev

CMD ["/bin/bash"]
