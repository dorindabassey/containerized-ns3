# Use official Fedora base image
FROM fedora:40

# Create a non-root user
RUN useradd -ms /bin/bash ns3user

# Install only necessary packages
RUN dnf -y update && dnf install -y \
    gcc gcc-c++ \
    python3 python3-pip \
    cmake git wget \
    iputils iproute \
    net-tools nmap \
    netcat tcpdump \
    && dnf clean all && rm -rf /var/cache/dnf

# Set working directory to user home
WORKDIR /home/ns3user

# Switch to non-root user
USER ns3user

# Clone and build NS-3
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git ns-3-dev
WORKDIR /home/ns3user/ns-3-dev
RUN git checkout -b ns-3.43-branch ns-3.43

# Configure and build NS-3
RUN ./ns3 configure --build-profile=debug --enable-examples --enable-tests && ./ns3 build

# Copy simulation scripts
COPY --chown=ns3user:ns3user scratch/ns3_app_onesubnet.cc ./scratch/
COPY --chown=ns3user:ns3user scratch/container-ns3-app.cc ./scratch/
COPY --chown=ns3user:ns3user scratch/container-ns3-app-security.cc ./scratch/
COPY --chown=ns3user:ns3user scratch/fqcodel.cc ./scratch/

# Rebuild with your simulation files
RUN ./ns3 build

# Add binaries to PATH for runtime use
ENV PATH="/home/ns3user/ns-3-dev/build:$PATH"

# Default shell
CMD ["/bin/bash"]
