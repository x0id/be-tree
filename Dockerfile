FROM debian:bookworm-slim

RUN apt-get update -qq
RUN apt-get install --no-install-recommends -y build-essential
RUN apt-get install --no-install-recommends -y libgsl-dev
RUN apt-get install --no-install-recommends -y valgrind
RUN apt-get install --no-install-recommends -y flex bison
