# Minimal Ubuntu-based container for building CSE on Linux
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    curl \
    unzip \
    ninja-build \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    xorg-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libosmesa6-dev \
    mesa-common-dev \
    mesa-utils \
  && rm -rf /var/lib/apt/lists/*

RUN curl -Ls https://astral.sh/uv/install.sh | sh && \
    /root/.local/bin/uv --version

ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /workspace

# Note: The source code is mounted via devcontainer.json mounts configuration
# so there's no need to COPY files here

CMD ["/bin/bash"]
