CC=gcc
INCLUDE=-I../include/ -I../../gc_benchmark/include/
CFLAGS=$(INCLUDE) -g -pg
DEPS=entaglement.h binary_tree.h cdouble_list.h cyclic_list.h gc_types.h gc_constants.h gc_shared.h gc_cheney.h gc_util.h garbage_collector.h gc_cheney_base.h benchmark.h
OBJ=main.o gc_types.o binary_tree.o cdouble_list.o cyclic_list.o gc_shared.o gc_util.o gc_cheney.o garbage_collector.o gc_cheney_base.o benchmark.o entanglement.o

%.o: ../src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
code_sweep_gen: $(OBJ)
	$(CC) -pg -o $@ $^ $(CFLAGS)
	rm *.o
	mv $@ ../bin/$@
