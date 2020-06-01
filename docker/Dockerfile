FROM ubuntu:14.04

RUN sed -i 's/# \(.*multiverse$\)/\1/g' /etc/apt/sources.list && \
    apt-get install software-properties-common -y && \
    add-apt-repository "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty main" && \
    apt-get update && \
    apt-get -y upgrade && \
    apt-get install -y --allow-unauthenticated \
        build-essential libexpat1 libexpat1-dev python3.4-dev \
        git man vim wget screen sudo telnet \
        llvm-3.4 clang-3.4 binutils-arm-none-eabi libncurses5-dev \
        mtools dosfstools

ENV HOME /root
WORKDIR /root

ADD scripts /root/scripts

# Compile and install QEMU
RUN apt-get install -y libtool pkg-config zlib1g-dev zlib1g libglib2.0-dev \
    libfdt-dev libpixman-1-dev
RUN cd /root && \
    git clone 'https://github.com/Zeke-OS/qemu.git' && \
    cd qemu && git checkout 'bf4eb7c8e705e997233415926fae83d31240e3b1' && \
    cp /root/scripts/install_qemu.sh /root/qemu/ && \
    cd /root/qemu && ./install_qemu.sh && \
    rm -rf /root/qemu

# Compile and install GDB
RUN apt-get install -y texinfo
COPY gdb-7.10.tar.xz /root/
RUN tar xvf gdb-7.10.tar.xz && rm gdb-7.10.tar.xz && \
    cp /root/scripts/install_gdb.sh /root/gdb-7.10/ && \
    cd /root/gdb-7.10 && ./install_gdb.sh && \
    rm -rf /root/gdb-7.10

RUN rm -rf /var/lib/apt/lists/*

ADD root /root

CMD ["bash"]
