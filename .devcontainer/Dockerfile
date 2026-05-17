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
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    xorg-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    mesa-common-dev \
    mesa-utils \
  && rm -rf /var/lib/apt/lists/*

RUN curl -Ls https://astral.sh/uv/install.sh | sh && \
    /root/.local/bin/uv --version

ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /workspace

COPY . /workspace

RUN chmod +x ./build.sh && ./build.sh

# Optional: run tests in build directory
# RUN cmake --build build --target test --parallel

CMD ["/bin/bash"]
