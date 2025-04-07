# Dockerfile for NS3
FROM fedora:40

ARG DEBIAN_FRONTEND=noninteractive
# Install prerequisites
RUN dnf -y update && dnf install -y gcc gcc-c++ python3 python3-pip cmake git wget iputils iproute net-tools nmap netcat tcpdump

# Set working directory
WORKDIR /tmp

# Download and install NS3
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git
WORKDIR /tmp/ns-3-dev/
RUN git checkout -b ns-3.43-branch ns-3.43

RUN rm -rf /tmp/ns-3-dev/cmake-cache
RUN ./ns3 configure  --build-profile=debug --enable-examples --enable-tests && ./ns3 build

# Add NS3 binaries to PATH
ENV PATH="/root/ns-3/build:$PATH"

# Copy simulation script
COPY scratch/ns3_app_onesubnet.cc /tmp/ns-3-dev/scratch/
COPY scratch/container-ns3-app.cc /tmp/ns-3-dev/scratch/
COPY scratch/container-ns3-app-security.cc /tmp/ns-3-dev/scratch/

# Build the simulation
RUN cd /tmp/ns-3-dev/ && ./ns3 build

# Run simulation by default
CMD ["bash"]
