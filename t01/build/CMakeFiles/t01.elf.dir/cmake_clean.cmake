file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "flash_project_args"
  "project_elf_src.c"
  "t01.bin"
  "t01.map"
  "CMakeFiles/t01.elf.dir/project_elf_src.c.obj"
  "project_elf_src.c"
  "t01.elf"
  "t01.elf.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/t01.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
