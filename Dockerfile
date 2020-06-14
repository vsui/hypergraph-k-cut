FROM ubuntu:bionic

# Install cmake. See https://apt.kitware.com
RUN apt-get update -yq && \
    apt-get install -yq apt-transport-https ca-certificates gnupg software-properties-common wget && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
    apt-get update -yq && \
    apt-get install -yq cmake

# TODO add support for choosing compiler
# Install clang-10
# I also use this logic in the workflow, maybe extract
ENV LLVM_OS_NAME bionic
ENV LLVM_VERSION 10
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    apt-add-repository "deb http://apt.llvm.org/${LLVM_OS_NAME}/ llvm-toolchain-${LLVM_OS_NAME}-${LLVM_VERSION} main" && \
    apt-get install -yq clang-10

COPY . /hypergraph
RUN cmake -H/hypergraph -Bbuild -DCMAKE_CXX_COMPILER=clang-10
RUN cmake --build /hypergraph --target test
ENTRYPOINT ["sh"]
