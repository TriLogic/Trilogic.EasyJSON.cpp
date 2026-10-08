FROM gcc:16.2.0-trixie

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build gdb git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
CMD ["bash"]
