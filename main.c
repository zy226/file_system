#include <stdio.h>

#include "disk.h"
#include "file.h"
#include "shell.h"


int main(){
    boot_load();
    fs_shell();
    return 0;
}