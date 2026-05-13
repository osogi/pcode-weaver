// gcc obfuscated2.c  -o obfuscated2.elf

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define UNIQUE_VAR(prefix) CONCAT(prefix, __COUNTER__)

int id(int arg){
    return arg;
}

int main(int argc, char** argv){
    int res = 0;
    if(id(1)) res = (id(argc) & id(1)) * id(2) + (id(argc) ^ id(1));
    if(id(1)) res = (id(argc) & id(1)) * id(2) + (id(1) ^ id(argc));
    if(id(1)) res = (id(1) & id(argc)) * id(2) + (id(argc) ^ id(1));
    if(id(1)) res = (id(1) & id(argc)) * id(2) + (id(1) ^ id(argc));
    if(id(1)) res = id(2) * (id(argc) & id(1)) + (id(argc) ^ id(1));
    if(id(1)) res = id(2) * (id(argc) & id(1)) + (id(1) ^ id(argc));
    if(id(1)) res = id(2) * (id(1) & id(argc)) + (id(argc) ^ id(1));
    if(id(1)) res = id(2) * (id(1) & id(argc)) + (id(1) ^ id(argc));
    if(id(1)) res = (id(argc) ^ id(1)) + (id(argc) & id(1)) * id(2);
    if(id(1)) res = (id(1) ^ id(argc)) + (id(argc) & id(1)) * id(2);
    if(id(1)) res = (id(argc) ^ id(1)) + (id(1) & id(argc)) * id(2);
    if(id(1)) res = (id(1) ^ id(argc)) + (id(1) & id(argc)) * id(2);
    if(id(1)) res = (id(argc) ^ id(1)) + id(2) * (id(argc) & id(1));
    if(id(1)) res = (id(1) ^ id(argc)) + id(2) * (id(argc) & id(1));
    if(id(1)) res = (id(argc) ^ id(1)) + id(2) * (id(1) & id(argc));
    if(id(1)) res = (id(1) ^ id(argc)) + id(2) * (id(1) & id(argc));

    return res;
}