# glibc compatibility without Ubuntu bloat
FROM debian:bookworm-slim

# Grab g++ and OpenSSL for compiling
RUN apt-get update && apt-get install -y g++ libssl-dev

# Set up our working directory
WORKDIR /snuau

# Copy all the C++ source files into the container
# The current directory is dictated by the build context for each container in the .yml file, NOT the position of the Dockerfile
COPY . .

# Compile both the node and the victim binaries
RUN g++ node.cpp lib/circularBuff.cpp -o node -Wall
RUN g++ victim.cpp public/MurmurHash3.cpp -o victim -lcrypto -Wall

# Default to running the relay node 
CMD ["./node"]