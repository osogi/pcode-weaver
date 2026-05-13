// gcc obfuscated.c  -o obfuscated.elf

int main(int argc, char** argv){
    return (argc ^ 1) + 2 * (argc & 1);
}