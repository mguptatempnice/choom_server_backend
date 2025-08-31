# Base image
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libgumbo-dev \
    libmicrohttpd-dev \
    nlohmann-json3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy source code
COPY . .

# Build project
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

# Expose ports
EXPOSE 8080

# Persistent disk path for SQLite DB
VOLUME ["/var/data"]

# Run indexer first (once), then start search server
CMD ["/app/build/choom_server", "--db", "/var/data/search.db"]
