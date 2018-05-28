Przed uruchomieniem testów należy wykonać:

    echo 10 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

Sposób użycia: make
Można też osobno skompilować testy: make all
Jak również uruchomić: make test


Przykład poprawnego wykonania testów:
    # make test
    echo 10 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
    timeout 10s sh -c ./test-mmap
    timeout 10s sh -c ./test-munmap
    timeout 10s sh -c ./test-mremap
    timeout 10s sh -c ./test-mprotect
    timeout 10s sh -c ./test-exec-simple
    timeout 10s sh -c ./test-dup-to-remote
    timeout 10s sh -c ./test-dup2-to-remote
    timeout 10s sh -c ./test-dup-from-remote
    timeout 10s sh -c ./test-cloexec
    timeout 10s sh -c ./test-close
    timeout 10s sh -c ./test-efault
    timeout 10s sh -c ./test-invalid
    timeout 10s sh -c ./test-hugetlb
    timeout 10s sh -c ./test-syscall-enter
    timeout 10s sh -c ./test-file-leak


Za każdy z testów można dostać 2/3 punkta.
