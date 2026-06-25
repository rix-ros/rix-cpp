# Bazel External Example

This example mirrors `simple_subscriber/` but using bazel to build.

## Files

- `BUILD.bazel`: The bazel build file. This file outlines how to build the project.
- `MODULE.bazel`: The bazel module file. This file outlines the project's dependencies.
- `.bazelrc`: The bazel configuration file. This file contains the project's default configuration.
- `rix.BUILD.bazel`: The bazel build file for the rix-cpp library. This file gets symlinked to your pre-installed `~/.rix` to package the library for bazel.
- `rix.bzl`: The bazel build rules for the rix-cpp library. This file specifies the location of .rix packages on your system.
- `third_party/`: The bazel external dependencies. This directory contains the `BUILD.bazel` files for the external dependencies. This allows bazel to use the preinstalled dependencies from `setup.bash`.

## Installation

Please install bazel using [bazelisk](https://bazel.build/install/bazelisk). Instructions for installing can be found in the [bazelisk github](https://github.com/bazelbuild/bazelisk) under _Requirements_.

1. Install golang

```bash
# Ubuntu. Please consult the official golang documentation for other distributions.
sudo apt install golang
# (Optional) Set GOPATH to be a dot-file to avoid polluting the home directory
go env -w GOPATH=$HOME/.go
echo 'export PATH=$PATH:$(go env GOPATH)/bin' >> ~/.bashrc
source ~/.bashrc
```

2. Install bazelisk

```bash
go install github.com/bazelbuild/bazelisk@latest
sudo ln -s $(go env GOPATH)/bin/bazelisk /usr/local/bin/bazel
```

## Usage

0. Enable autocompletion for bazel

```bash
# Bash
source <(bazel help completion bash)
```

1. Build the example

```bash
bazel build //...
```

2. Run the example

```bash
bazel run //:simple_subscriber
```

## Requirements

- rix-cpp library
- C++17 or newer
- bazelisk

See the main project README for setup instructions.
