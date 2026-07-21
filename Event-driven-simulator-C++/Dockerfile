FROM ubuntu:22.04

# Avoid prompts during apt installs
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++-11 \
    python3.10 \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set default compiler
ENV CC=gcc-11
ENV CXX=g++-11

# Install Python plotting dependencies
RUN pip3 install matplotlib pandas seaborn

WORKDIR /app
COPY . /app

# Build simulator in Release mode
RUN cmake -B build_release -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build_release -j $(nproc)

CMD ["/bin/bash"]
