# covarray development image: GCC 16 (official image, Debian 13 "trixie") + Clang/LLVM 23 + tooling
FROM gcc:16-trixie

ARG LLVM_VERSION=23
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000
ENV DEBIAN_FRONTEND=noninteractive

# Base build, debug and profiling tools
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl gnupg git git-lfs sudo less \
        zip unzip tar pkg-config \
        cmake ninja-build ccache \
        gdb valgrind heaptrack linux-perf strace hyperfine \
        cppcheck doxygen graphviz \
        python3 python3-venv gh \
    && rm -rf /var/lib/apt/lists/*

# Clang/LLVM from apt.llvm.org (compiler, clangd, clang-tidy, clang-format, lld, lldb, libc++,
# and libclang-rt: the sanitizer runtimes that -fsanitize=address,undefined links against)
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/trixie/ llvm-toolchain-trixie-${LLVM_VERSION} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        clang-${LLVM_VERSION} clangd-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clang-format-${LLVM_VERSION} \
        lld-${LLVM_VERSION} lldb-${LLVM_VERSION} llvm-${LLVM_VERSION} \
        libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev libclang-rt-${LLVM_VERSION}-dev \
    && for t in clang clang++ clangd clang-tidy clang-format lld lldb llvm-cov llvm-profdata llvm-symbolizer; do \
           ln -sf /usr/bin/$t-${LLVM_VERSION} /usr/local/bin/$t; done \
    && rm -rf /var/lib/apt/lists/*

# vcpkg (manifest mode in the repo; binary cache lives in a volume).
# Full clone, not --depth 1: vcpkg needs the history to resolve the builtin-baseline in vcpkg.json
ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git ${VCPKG_ROOT} \
    && ${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics

# uv for Python versions, virtual environments and tools (pin a version tag in real use)
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

# Non-root user that matches your host UID so files in the workspace keep your ownership
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash ${USERNAME} \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chown -R ${USERNAME}:${USERNAME} ${VCPKG_ROOT}

USER ${USERNAME}
ENV PATH=/home/${USERNAME}/.local/bin:${PATH} \
    CCACHE_DIR=/home/${USERNAME}/.cache/ccache \
    VCPKG_DEFAULT_BINARY_CACHE=/home/${USERNAME}/.cache/vcpkg
RUN mkdir -p ${CCACHE_DIR} ${VCPKG_DEFAULT_BINARY_CACHE} /home/${USERNAME}/.cache/uv /home/${USERNAME}/.claude

# Claude Code CLI via Anthropic's native installer (no Node.js needed; installs to ~/.local/bin)
RUN curl -fsSL https://claude.ai/install.sh | bash
