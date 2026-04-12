# ==============================================================================
# Docker Build & Run Instructions
# ==============================================================================
# 1. Build the image (run from the project root directory):
#    docker build -t ai_txt -f data/Dockerfile .
#
# 2. Run the container:
#    docker run -d --name ai_txt -p 18080:18080 -v /home/Docker/webapp/etc/ai:/webapp/etc/ai ai_txt
# ==============================================================================

FROM ubuntu:25.10

# Set non-interactive configuration
ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies (like libcurl for making HTTP requests, and libstdc++ for C++)
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libstdc++6 \
    libc6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*


# Set up the working directory inside the container
WORKDIR /app

# Ensure configuration directory exists
RUN mkdir -p /webapp/etc/ai

# Copy the pre-built microservice binary from the host
# (Assumes the binary is located at build/ai_srv in the project root)
COPY build/ai_srv /usr/local/bin/ai_srv

# Expose the configuration port (18080)
EXPOSE 18080

# Run the microservice and point to the ENV file location
CMD ["ai_srv", "--env", "/webapp/etc/ai/ai_txt.env"]
