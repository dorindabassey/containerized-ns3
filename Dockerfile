# Use a base image for NS3
FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    git \
    cmake \
    && apt-get clean

# Set the working directory
WORKDIR /ns-3-dev

# Copy your NS-3 workspace into the container
COPY ./ns-3-dev /ns-3-dev

# Set the entry point to bash
# ENTRYPOINT ["bash"]

