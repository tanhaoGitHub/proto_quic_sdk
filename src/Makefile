#.PHONY:quic
CC=g++
CFLAGS=-Wall --std=c++11 -fpermissive -c -g -O0
INC=-I . -I ../libquic/include
LDFLAGS=-L ../libquic/ -lbase -lbase_i18n -lboringssl -lcrcrypto -licui18n -licuuc -lnet -lprotobuf_lite -lurl -lpthread -lrt



#filter_out_files = quic_test.o
SRCFILES= $(wildcard ./*.cc)

OBJFILES1=$(SRCFILES:.cc=.o)
OBJFILES=$(filter-out $filter_out_files,$(OBJFILES1))

quic: $(OBJFILES) 
	ar -r libflyquic.a $(OBJFILES) 


.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

print-%:
	@echo $* = $($*)

clean:
	rm $(OBJFILES) libflyquic.a
#libquic.a  libssl.a libprotobuf.a libcrypto.a
