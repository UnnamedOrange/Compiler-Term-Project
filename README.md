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

  CMake configure and build.

  Example:

  ```shell
  ./build.sh
  ```

- `koopa.sh`

  Call `compiler -koopa ... -o ...` manually using cases in [case](./case) folder. You only need to specify ID. Should call `build.sh` beforehand.

  Example:

  ```shell
  ./koopa.sh 1 # Use 001-main.c.
  ```

- `riscv.sh`

  Call `compiler -riscv ... -o ...` manually using cases in [case](./case) folder. You only need to specify ID. Should call `build.sh` beforehand.

  Example:

  ```shell
  ./riscv.sh 1 # Use 001-main.c.
  ```

- `compiler.sh`

  Use your compiler in RISC-V mode and execute the output file. Should call `build.sh` beforehand. Used only in Docker command line.

  Example:

  ```shell
  ./compiler.sh case/001-main.sy
  ```

## License

Copyright (c) UnnamedOrange. Licensed under the MIT License.

See the [LICENSE](./LICENSE) file in the repository root for full license text.

## Credits

### [fmtlib](https://github.com/fmtlib)/[fmt](https://github.com/fmtlib/fmt)

```
Copyright (c) 2012 - present, Victor Zverovich

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--- Optional exception to the license ---

As an exception, if, as a result of your compiling your source code, portions of this Software are embedded into a machine-executable object form of such source code, you may redistribute such embedded portions in such object form without including the above copyright and permission notices.
```

### [p-ranav](https://github.com/p-ranav)/[argparse](https://github.com/p-ranav/argparse)

```
Copyright (c) 2018 Pranav Srinivas Kumar <pranav.srinivas.kumar@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

