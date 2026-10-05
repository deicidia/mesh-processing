ARG BASE_IMAGE=ubuntu:24.04
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    clang-18 \
    clang-tools-18 \
    g++-14 \
    libstdc++-14-dev \
    git \
    wget \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    && update-alternatives --install /usr/bin/clang-scan-deps clang-scan-deps /usr/bin/clang-scan-deps-18 100 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

ARG PRESET=default
ENV PRESET=${PRESET}

# Build according to the specified PRESET (default, hip, cuda)
RUN cmake --preset ${PRESET} && cmake --build --preset ${PRESET}

# Symlink to the active preset executable for a unified entrypoint
RUN if [ -f "./build-${PRESET}/app" ]; then ln -s "/app/build-${PRESET}/app" /app/app_bin; else ln -s /app/build/app /app/app_bin; fi

ENTRYPOINT ["/app/app_bin"]
CMD ["data/unit_square_132.meshb"]
