#include "my_malloc.h"
#include <stdio.h>

#define get_ptr_int(bag) (int*)(bag->addr)

int main(){
    bag *a_bag = my_malloc(40);
    bag *b_bag = my_malloc(80);
    bag *c_bag = my_malloc(32);
    bag *d_bag = my_malloc(10000000);
    bag *e_bag = my_malloc(55);
    printf("test\n");
    printf("*a : %d, a : %p\n",*(get_ptr_int(a_bag)),get_ptr_int(a_bag));

    *get_ptr_int(a_bag) = 7;
    printf("*a : %d, a : %p\n",*(get_ptr_int(a_bag)),get_ptr_int(a_bag));
    printf("*b : %d, b : %p\n",*(get_ptr_int(b_bag)),get_ptr_int(b_bag));
    printf("*c : %d, c : %p\n",*(get_ptr_int(c_bag)),get_ptr_int(c_bag));
    printf("*d : %d, d : %p\n",*(get_ptr_int(d_bag)),get_ptr_int(d_bag));
    printf("*e : %d, e : %p\n",*(get_ptr_int(e_bag)),get_ptr_int(e_bag));

    printf("myfree(b)\n");
    my_free(b_bag);
    printf("myfree(c)\n");
    my_free(c_bag);
    printf("myfree(d)\n");
    my_free(d_bag);
    printf("defragmentation()\n");
    defragmentation();
    printf("*a : %d, a : %p\n",*(get_ptr_int(a_bag)),get_ptr_int(a_bag));
    printf("*e : %d, e : %p\n",*(get_ptr_int(e_bag)),get_ptr_int(e_bag));
    b_bag = my_malloc(80);
    c_bag = my_malloc(32);
    d_bag = my_malloc(120);
    printf("*b : %d, b : %p\n",*(get_ptr_int(b_bag)),get_ptr_int(b_bag));
    printf("*c : %d, c : %p\n",*(get_ptr_int(c_bag)),get_ptr_int(c_bag));
    printf("*d : %d, d : %p\n",*(get_ptr_int(d_bag)),get_ptr_int(d_bag));
}