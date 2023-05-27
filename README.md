# **voinst**

[![Build-and-Test](https://github.com/BlurringShadow/voinst/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/BlurringShadow/voinst/actions/workflows/build-and-test.yml)
[![wakatime](https://wakatime.com/badge/user/2f4337be-cbd2-4165-a44e-e1c8d69c1644/project/e8bd6cb7-af68-4909-a817-a3a4d98eb304.svg)](https://wakatime.com/badge/user/2f4337be-cbd2-4165-a44e-e1c8d69c1644/project/e8bd6cb7-af68-4909-a817-a3a4d98eb304)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/060a0f776cb74c29a262cb45b75d65d7)](https://app.codacy.com/gh/BlurringShadow/voinst/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/060a0f776cb74c29a262cb45b75d65d7)](https://app.codacy.com/gh/BlurringShadow/voinst/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_coverage)

<span style="font-size:larger;"> **Make all data in memory observable.** </span>

<br/>

## Getting Started

### Prerequisites

- **CMake v3.22+** - required for building

- **C++ Compiler** - needs to support **C++23** standard, i.e. _MSVC_, _GCC_, _Clang_. You could checkout [github workflow file](.github/workflows/build.yml) for suitable compilers.

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
