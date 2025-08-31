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

# Expose both ports (8080 search, 8081 indexer)
EXPOSE 8080 8081

# Persistent disk path on Render will be /var/data
# We make sure it's available for both indexer and search server
VOLUME ["/var/data"]

# Start both servers; indexer in background, search in foreground
CMD [ "sh", "-c", "/app/build/indexer_server --db /var/data/search.db & /app/build/search_server --db /var/data/search.db" ]
