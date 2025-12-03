FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
	build-essential \
	gdb \
	valgrind \
    libcriterion-dev \ 
    clang-format

CMD ["/bin/bash"]
