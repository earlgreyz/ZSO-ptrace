TESTS = \
		test-mmap \
		test-munmap \
		test-mremap \
		test-mprotect \
		test-exec-simple \
		test-dup-to-remote \
		test-dup2-to-remote \
		test-dup-from-remote \
		test-cloexec \
		test-close \
		test-efault \
		test-invalid \
		test-hugetlb \
		test-syscall-enter \
		test-file-leak \
		$(null)

default: test
all: $(TESTS)

CFLAGS = -g -Wall -Wextra
LDFLAGS =

test: prepare $(TESTS:%=%.test)
all: $(TESTS) true-static.bin

prepare:
	-echo 10 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

%.test: %
	-timeout 10s sh -c ./$<

.SECONDARY: $(TESTS)
test%: test%.o utils.o
	$(CC) $(LDFLAGS) -o $@ $^

true-static.bin: true-static.o
	objcopy --dump-section .text=$@ $^

clean:
	rm -f $(TESTS) *.o true-static.bin
