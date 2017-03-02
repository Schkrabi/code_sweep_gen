#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "binary_tree.h"
#include "gc_types.h"
#include "gc_constants.h"
#include "cyclic_list.h"
#include "cdouble_list.h"
#include "entanglement.h"

#define XGEN_TYPE_TABLE(x)          \
            x(void, TYPE_PTR)       \
            x(btree, TYPE_BTREE_T)  \
            x(clist, TYPE_CLIST_T)  \
            x(cdlist, TYPE_CDLIST_T)\
            x(double, TYPE_DOUBLE)  \
            x(int, TYPE_INT)        \
            x(test_struct, TYPE_TEST_STRUCT_T) \
            x(entanglement, TYPE_ENTANGLEMENT_T)
            
            
#define DESC_VAR_NAME(TYPE) TYPE ## _descriptor
#define XGEN_MAKE_DESCRIPTOR_VARIABLES(TYPE, NUM) type_info_t DESC_VAR_NAME(TYPE);
#define XGEN_MAKE_DESCRIPTORS(TYPE, NUM) TYPE ## _make_descriptor(&DESC_VAR_NAME(TYPE));
#define XGEN_GC_SCAN_STRUCT(TYPE, NUM) len += make_gc_scan_struct_code_per_type(out + len, &DESC_VAR_NAME(TYPE), NUM, 1);
#define XGEN_GC_SCAN_PTR(TYPE, NUM) len += make_gc_scan_ptr_code_per_type(out + len, &DESC_VAR_NAME(TYPE), NUM, 4);
#define XGEN_GC_WALK_ARRAY(TYPE, NUM) len += make_gc_walk_array_per_type(out + len, &DESC_VAR_NAME(TYPE), NUM, 2);
            
#define PART_FILE_PATH "/home/schkrabi/Dokumenty/C/code_sweep_gen/gc_custom._const_part.c"
#define GEN_FILE_PATH "/home/schkrabi/Dokumenty/C/code_sweep_gen/gc_custom.c"
#define BUFF_SIZE 10240 //10 KB

int void_make_descriptor(type_info_t *info);
int double_make_descriptor(type_info_t *info);
int test_struct_make_descriptor(type_info_t *info);
int int_make_descriptor(type_info_t *info);
int make_gc_scan_struct_code_per_type(char *out, type_info_t *info, int type_num, size_t indent);
int make_gc_scan_ptr_code_per_type(char *out, type_info_t *info, int type_num, size_t indent);
int add_code_row(char *out, size_t indent, const char* fstr, ...);
int make_gc_scan_ptr_code(char *out);
int make_gc_scan_struct_code(char *out);
int make_gc_walk_array(char *out);
int make_gc_walk_array_per_type(char *out,type_info_t *info, int type_num, size_t indent);

size_t atom_alloc_size(type_info_t *info);

int main(int agrc, char* argv[])
{
    char buff[BUFF_SIZE];
    FILE    *in, *out;
    size_t len;
    
    in = fopen(PART_FILE_PATH, "r");
    
    if(in == NULL)
    {
        printf("input file open error\n");
        return 1;
    }
    
    out = fopen(GEN_FILE_PATH, "w");
    
    if(out == NULL)
    {
        printf("output file open error\n");
        return 1;
    }
    
    while(!feof(in))
    {
        fgets(buff, BUFF_SIZE, in);
        fprintf(out, "%s", buff);
    }
    
    fprintf(out, "\n\n");
    
    len = make_gc_scan_ptr_code(buff);
    fprintf(out, "%s\n", buff);
    len = make_gc_scan_struct_code(buff);
    fprintf(out, "%s\n", buff);
    len = make_gc_walk_array(buff);
    fprintf(out, "%s", buff);
    
    fclose(in);
    fclose(out);
    
    return 0;
}

int void_make_descriptor(type_info_t *info)
{
    info->size = sizeof(void*);
    info->number_of_references = 1;
    info->offsets = (unsigned long*)malloc(1*sizeof(unsigned long));
    info->offsets[0] = 0;
    
    return 0;
}

int double_make_descriptor(type_info_t *info)
{
    info->size = sizeof(double);
    info->number_of_references = 0;
    info->offsets = NULL;
    
    return 0;
}
int int_make_descriptor(type_info_t *info)
{
    info->size = sizeof(int);
    info->number_of_references = 0;
    info->offsets = NULL;
    
    return 0;
}

int test_struct_make_descriptor(type_info_t *info)
{
    test_struct_t test_instance;
    info->size = sizeof(test_struct_t);
    info->number_of_references = 2;
    info->offsets = malloc(2*sizeof(unsigned long));
    info->offsets[0] = (unsigned long)&test_instance.ptr1 - (unsigned long)&test_instance;
    info->offsets[1] = (unsigned long)&test_instance.ptr2 - (unsigned long)&test_instance;
    
    return 0;
}

int add_code_row(char *out, size_t indent, const char* fstr, ...)
{
    va_list args;
    size_t len, i;
    
    va_start(args, fstr);
    len = 0;
    for(i = 0; i < indent; i++)
    {
        len += sprintf(out + len, "\t");
    }
    
    len += vsprintf(out + len, fstr, args);
    
    len += sprintf(out + len, "\n");
    
    return len;    
}

int make_gc_scan_struct_code_per_type(char *out, type_info_t *info, int type_num, size_t indent)
{
    int i;
    size_t len;
    
    len = 0;
    if(info->number_of_references > 0)
    {
        len = add_code_row(out + len, indent, "case %d:", type_num);
        
        for(i = 0; i < info->number_of_references; i++)
        {
            size_t  offset;
            
            offset = info->offsets[i];
            len += add_code_row(out + len, indent + 1, "*(void**)(ptr + %u) = gc_custom_scan_ptr(*(void**)(ptr + %u));", (unsigned)offset, (unsigned)offset);
        }
        len += add_code_row(out + len, indent + 1, "break;");
    }
    
    return len;
}

int make_gc_scan_struct_code(char *out)
{
    size_t len;
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTOR_VARIABLES)
    
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTORS)
    len = 0;
    
    len += add_code_row(out + len, 0, "int gc_custom_scan_struct(void *ptr, int type)");
    len += add_code_row(out + len, 0, "{");
    len += add_code_row(out + len, 1, "switch(type)");
    len += add_code_row(out + len, 1, "{");
    
    XGEN_TYPE_TABLE(XGEN_GC_SCAN_STRUCT)
    
    len += add_code_row(out + len, 1, "}");
    len += add_code_row(out + len, 1, "return 0;");
    len += add_code_row(out + len, 0, "}");
}

int make_gc_scan_ptr_code_per_type(char *out, type_info_t *info, int type_num, size_t indent)
{
    size_t len = 0;
    
    len += add_code_row(out + len, indent, "case %d:", type_num);
    len += add_code_row(out + len, indent + 1, "if(block_is_array(block))");
    len += add_code_row(out + len, indent + 1, "{");
    len += add_code_row(out + len, indent + 2, "byte_size = block_get_array_size(block) * %d;", (unsigned)(info->size));
    len += add_code_row(out + len, indent + 2, "dst = split_block(&gc_cheney_base_remaining_to_space, byte_size);");
    len += add_code_row(out + len, indent + 2, "memcpy(dst, block, byte_size + %u);", (unsigned)(sizeof(block_t)));
    len += add_code_row(out + len, indent + 1, "}");
    len += add_code_row(out + len, indent + 1, "else");
    len += add_code_row(out + len, indent + 1, "{");
    len += add_code_row(out + len, indent + 2, "dst = split_block(&gc_cheney_base_remaining_to_space, %d);", atom_alloc_size(info));
    len += add_code_row(out + len, indent + 2, "memcpy(dst, block, %u);", (unsigned)(atom_alloc_size(info) + sizeof(block_t)));
    len += add_code_row(out + len, indent + 1, "}");
    len += add_code_row(out + len, indent + 1, "break;");
    
    return len;
}

int make_gc_scan_ptr_code(char *out)
{
    size_t  len;
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTOR_VARIABLES)
    
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTORS)
    
    len = 0;
    len += add_code_row(out + len, 0, "void *gc_custom_scan_ptr(void *ptr)");
    len += add_code_row(out + len, 0, "{");
    len += add_code_row(out + len, 1, "block_t *block;");
    len += add_code_row(out + len, 1, "for(block = gc_cheney_base_from_space; block < gc_cheney_base_remaining_block; block = next_block(block))");
    len += add_code_row(out + len, 1, "{");
    len += add_code_row(out + len, 2, "if(is_pointer_to(block, ptr))");
    len += add_code_row(out + len, 2, "{");
    len += add_code_row(out + len, 3, "if(!block_has_forward(block))");
    len += add_code_row(out + len, 3, "{");
    len += add_code_row(out + len, 4, "block_t *dst;");
    len += add_code_row(out + len, 4, "size_t byte_size;");
    len += add_code_row(out + len, 4, "switch(block_get_type(block))");
    len += add_code_row(out + len, 4, "{");
    
    XGEN_TYPE_TABLE(XGEN_GC_SCAN_PTR)
    
    len += add_code_row(out + len, 4, "default:"); //Default behaviour as std cheney, should not be hit
    len += add_code_row(out + len, 4, "{");
    len += add_code_row(out + len, 5, "size_t block_size = block_get_size(block);");
    len += add_code_row(out + len, 5, "dst = split_block(&gc_cheney_base_remaining_to_space, block_size - sizeof(block_t));");
    len += add_code_row(out + len, 5, "memcpy(dst, block, block_size);");
    len += add_code_row(out + len, 4, "}");
    
    len += add_code_row(out + len, 4, "}");
    len += add_code_row(out + len, 4, "block_set_forward(block, dst);");
    len += add_code_row(out + len, 4, "return dst;");
    len += add_code_row(out + len, 3, "}");
    len += add_code_row(out + len, 3, "else");
    len += add_code_row(out + len, 4, "return gc_cheney_base_get_forwarding_addr(ptr, block, block_get_forward(block));");
    len += add_code_row(out + len, 2, "}");
    len += add_code_row(out + len, 1, "}");
    len += add_code_row(out + len, 1, "return NULL;");
    len += add_code_row(out + len, 0, "}");
    
    return len;    
}

int make_gc_walk_array_per_type(char *out,type_info_t *info, int type_num, size_t indent)
{
    size_t len = 0;
    
    if(info->number_of_references > 0)
    {
        len += add_code_row(out + len, indent    , "case %d:", type_num);
        len += add_code_row(out + len, indent + 1, "for(ptr = get_data_start(block); ptr < get_data_end(block); ptr += %u)", (unsigned)info->size);
        len += add_code_row(out + len, indent + 2, "gc_custom_scan_struct(ptr, %d);", type_num);
        len += add_code_row(out + len, indent + 1, "break;");
    }
    
    return len;
}

int make_gc_walk_array(char *out)
{
    size_t  len;
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTOR_VARIABLES)
    
    XGEN_TYPE_TABLE(XGEN_MAKE_DESCRIPTORS)
    
    len += add_code_row(out + len, 0, "int gc_custom_walk_array(block_t *block)");
    len += add_code_row(out + len, 0, "{");
    
    len += add_code_row(out + len, 1, "if(block_is_struct_block(block))"); //TODO This might be not necessary as non-struct options are excluded durign code generation
    len += add_code_row(out + len, 1, "{");
    len += add_code_row(out + len, 2, "void *ptr;");
    len += add_code_row(out + len, 2, "uint64_t type;");
    len += add_code_row(out + len, 2, "type = block_get_type(block);");
    len += add_code_row(out + len, 2, "switch(type)");
    len += add_code_row(out + len, 2, "{");

    XGEN_TYPE_TABLE(XGEN_GC_WALK_ARRAY)
    
    len += add_code_row(out + len, 2, "}"); 
    len += add_code_row(out + len, 1, "}");
    len += add_code_row(out + len, 0, "}");
    
    len = 0;
    
    return len;
}

size_t atom_alloc_size(type_info_t *info)
{
    if(info->size < sizeof(uint64_t))
    {
        return 0;
    }
    return info->size - sizeof(uint64_t);
}
