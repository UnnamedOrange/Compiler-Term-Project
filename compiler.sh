build/compiler -riscv ${1} -o build/a.S
clang build/a.S -c -o build/a.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
ld.lld build/a.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o build/a
qemu-riscv32-static build/a; echo $?
