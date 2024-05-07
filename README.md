# cppcia

[![ci](https://github.com/FeignClaims/cppcia/actions/workflows/ci.yml/badge.svg)](https://github.com/FeignClaims/cppcia/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/FeignClaims/cppcia/graph/badge.svg?token=1SACJV6X5E)](https://codecov.io/gh/FeignClaims/cppcia)

## About

cppcia is a tool to find C++ change impact according to files, locations or symbol names.

## Build

To build this repository, please follow instructions in [FeignClaims/cmake_starter_template](https://github.com/FeignClaims/cmake_starter_template) and [FeignClaims/customized_conan_recipes](https://github.com/FeignClaims/customized_conan_recipes).

In a nutshell, you should clone [FeignClaims/customized_conan_recipes](https://github.com/FeignClaims/customized_conan_recipes) and add the repository to the conan remote:

```bash
git clone https://github.com/FeignClaims/customized_conan_recipes.git
conan remote add <remote_name> <remote_path>
```

After that, run conan and cmake as normal:

```bash
# In the repository folder
conan install . [your_args..]
cmake --preset <preset_generated_by_conan>
```

## LICENSE

[UNLICENSED](LICENSE)
