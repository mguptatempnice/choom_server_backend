# Use a slim version of Ubuntu for a smaller final image size
FROM ubuntu:22.04 AS builder

# Set non-interactive frontend to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install all necessary build tools and library dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libmicrohttpd-dev \
    nlohmann-json3-dev \
    libgumbo-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the entire project source code into the container
COPY . .

# Build the project
# This creates the single executable in /app/build/
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)


# --- Final Stage ---
# Use a minimal base image for the final container to reduce size and improve security.
FROM ubuntu:22.04

# Set non-interactive frontend
ENV DEBIAN_FRONTEND=noninteractive

# Install only the RUNTIME dependencies needed for the executable.
# We don't need cmake, g++, or -dev packages in the final image.
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libsqlite3-0 \
    libmicrohttpd12 \
    libgumbo1 \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user for better security
RUN useradd -ms /bin/bash appuser
USER appuser
WORKDIR /home/appuser/app

# Copy ONLY the compiled executable from the builder stage.
# Also copy the frontend directory.
COPY --from=builder /app/build/choom_server .
COPY --from=builder /app/frontend ./frontend

# Expose the port the server will listen on.
# Platforms like Render will use this to route traffic.
EXPOSE 8080

# The command to run when the container starts.
# It will execute your single, unified server.
CMD ["./choom_server"]