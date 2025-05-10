FROM ruby:3.4.3-bullseye

RUN set -eux ; \
    apt-get update -y \
    && apt-get install -y --no-install-recommends \
      ninja-build build-essential libcairo2-dev libfreetype-dev libsdl2-dev \
      curl wget software-properties-common gnupg vim

RUN set -eux ; \
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
 && echo 'deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-15 main' >> /etc/apt/sources.list.d/llvm.list \
 && echo 'deb-src http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-15 main' >> /etc/apt/sources.list.d/llvm.list \
 && apt-get update -y \
 && apt-get install -y --no-install-recommends clang-15

WORKDIR /app

ADD Gemfile Gemfile.lock /app/
RUN bundle install

ADD ext /app/ext
ADD build /app/build
RUN ./build/build.rb > build.ninja && ninja

ADD gen.rb /app/
ADD lib /app/lib

ADD assets /app/assets

ENV XDG_RUNTIME_DIR=/root/xdg
RUN mkdir $XDG_RUNTIME_DIR

ENTRYPOINT ["/bin/bash"]
