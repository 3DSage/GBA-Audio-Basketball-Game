path=C:\devkitadv\bin
@ECHO OFF
SET VAR=0

gcc -c -O3 -mthumb-interwork main.c
gcc -marm -mthumb-interwork -o main.elf main.o
objcopy -O binary main.elf main.gba
del main.o main.elf &start main.gba
