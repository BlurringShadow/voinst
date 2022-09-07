# observable memory

[![Build-and-Test](https://github.com/BlurringShadow/observable_memory/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/BlurringShadow/observable_memory/actions/workflows/build-and-test.yml)
![wakatime](https://wakatime.com/badge/github/BlurringShadow/observable-memory.svg)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/060a0f776cb74c29a262cb45b75d65d7)](https://www.codacy.com/gh/BlurringShadow/observable_memory/dashboard?utm_source=github.com&utm_medium=referral&utm_content=BlurringShadow/observable_memory&utm_campaign=Badge_Grade)

Make all data in memory to be observable.

<br/>

## Getting Started

### Prerequisites

- **CMake v3.22+** - required for building

- **C++ Compiler** - needs to support at least the **C++20** and **partial C++23** standard, i.e. _MSVC_, _GCC_, _Clang_. You could checkout [github workflow file](.github/workflows/build.yml) for suitable compilers.

- **Vcpkg or Other Suitable Dependencies Manager** - this project uses vcpkg manifest to maintain dependencies. Checkout the
  [vcpkg.json](vcpkg.json) for required dependencies.

<br/>

### Installing

- Clone the project and use cmake to install, or
- Add [my vcpkg registry](https://github.com/BlurringShadow/vcpkg-registry) to your vcpkg-configuration.json.

<br/>

### Building the project

Use cmake to build the project, checkout [github workflow file](.github/workflows/build.yml) for details.

<br/>

## License

This project is licensed under the [Unlicense](https://unlicense.org/) license - see the
[LICENSE](LICENSE) file for details
