ARG UBUNTU_VERSION=22.04
# Use the official ubuntu:18.04 image as the parent image
FROM ubuntu:${UBUNTU_VERSION}

# Set the working directory to /app
WORKDIR /app

ARG UBUNTU_VERSION
ARG ARCH=amd64

ADD setup-ubuntu.sh /app
ADD sources.list /app

# Tell Ubuntu non-interactive install
ARG DEBIAN_FRONTEND=noninteractive

RUN echo ubuntu version ${UBUNTU_VERSION}

RUN bash ./setup-ubuntu.sh ${ARCH} ${UBUNTU_VERSION}

RUN apt-get install -y ca-certificates
RUN update-ca-certificates