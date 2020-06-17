# TODO maybe alpine linux instead
FROM ubuntu:bionic

# Install cmake. See https://apt.kitware.com
RUN apt-get update -yq && \
    apt-get install -yq apt-transport-https ca-certificates gnupg software-properties-common wget && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
    apt-get update -yq && \
    apt-get install -yq cmake
# TODO rm -rf /var/lib/apt/lists/* (need to see if I need to install any other dependencies

# TODO Git is necessary during the configuration phase of cmake. Either add this to the other installs
# or find a way to remove this dependency
RUN apt-get update -yq && apt-get install -yq git

# TODO add support for choosing compiler
# Install clang-10
# I also use this logic in the workflow, maybe extract
ENV LLVM_OS_NAME bionic
ENV LLVM_VERSION 10
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    apt-add-repository "deb http://apt.llvm.org/${LLVM_OS_NAME}/ llvm-toolchain-${LLVM_OS_NAME}-${LLVM_VERSION} main" && \
    apt-get install -yq clang-10

# Install gcc-10
RUN apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
RUN apt-get install -yq g++-10

# Install dependencies separately so we don't have to repeat this layer
COPY install-deps.sh .
RUN ./install-deps.sh

RUN mkdir /hypergraph
COPY CMakeLists.txt /hypergraph
COPY cmake/ /hypergraph/cmake
COPY app/ /hypergraph/app
COPY lib/ /hypergraph/lib
COPY ext/ /hypergraph/ext
COPY scripts/ /hypergraph/scripts

WORKDIR /hypergraph

RUN cmake -H. -Bbuild -DCMAKE_CXX_COMPILER=clang++-10
RUN cmake --build build --target test -j8
RUN cmake --build build --target all -j8

# TODO add entrypoint that lets us use the installed binaries easily. Not sure if this is possible since
#      the script would need to be able to files from the host.
