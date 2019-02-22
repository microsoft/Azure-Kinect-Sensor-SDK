# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

FROM microsoft/vsts-agent:ubuntu-16.04-standard

# packages for building remotely with visual studio
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    gdb \
    gdbserver \
    git \
    openssh-server \
    rsync \
    sudo \
    wget

COPY ./bootstrap-ubuntu.sh /tmp/bootstrap-ubuntu.sh

RUN bash /tmp/bootstrap-ubuntu.sh

# configure SSH server
# https://docs.docker.com/engine/examples/running_ssh_service/#build-an-eg_sshd-image
RUN mkdir /var/run/sshd; \
    echo 'root:kinect' | chpasswd; \
    sed -i 's/PermitRootLogin .*/PermitRootLogin yes/' /etc/ssh/sshd_config; \
    sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

# Add a normal user
RUN useradd -ms /bin/bash k4a && echo 'k4a:kinect' | chpasswd && usermod -aG sudo k4a

# Grant permissions to the paths we will map the volumes to
RUN mkdir /var/tmp/build && mkdir /var/tmp/src && chmod o+w /var/tmp/*

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]

