FROM ubuntu:22.04

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libvulkan-dev \
    vulkan-tools \
    glslc-tools \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy source
COPY . .

# Build
RUN cmake -B build -G Ninja && \
    cmake --build build

# Shaders are copied next to the executable by CMake POST_BUILD
# Binary is at /workspace/build/voxel

CMD ["/workspace/build/voxel"]
