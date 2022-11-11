# Compiler-Term-Project

Compiler Term Project 2022

## Utilities

Built-in utilities:

- `autotest`

  Used only in Docker command line.

  Example:

  ```shell
  autotest -koopa -s lv1 .
  ```

  ```shell
  autotest -riscv -s lv1 .
  ```

  ```
  autotest -koopa .
  ```

  ```
  autotest -koopa .
  ```

Additional scripts:

- `build.sh`

  CMake configure and build. Used only in Docker command line.

  Example:

  ```shell
  ./build.sh
  ```

- `koopa.sh`

  Call `compiler -koopa ... -o ...` manually using cases in [case](./case) folder. You only need to specify ID. Should call `build.sh` beforehand. Used only in Docker command line.

  Example:

  ```shell
  ./koopa.sh 1 # Use 001-main.c.
  ```

- `riscv.sh`

  Call `compiler -riscv ... -o ...` manually using cases in [case](./case) folder. You only need to specify ID. Should call `build.sh` beforehand. Used only in Docker command line.

  Example:

  ```shell
  ./riscv.sh 1 # Use 001-main.c.
  ```

## License

Copyright (c) UnnamedOrange. Licensed under the MIT License.

See the [LICENSE](./LICENSE) file in the repository root for full license text.