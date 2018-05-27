# ZSO Zadanie 3: modyfikacja jądra

## Materiały dodatkowe
* [ptrace_remote.h](http://students.mimuw.edu.pl/ZSO/PUBLIC-SO/2017-2018/_build/html/_downloads/ptrace_remote.h)
* [z3_test.tar](http://students.mimuw.edu.pl/ZSO/PUBLIC-SO/2017-2018/_build/html/_downloads/z3_test.tar)

## Wprowadzenie
Do śledzenia programów użytkownika w systemie Linux służy syscall `ptrace`. Ma on (m.in.) następujące możliwości:

* podpięcie do procesu śledzonego (targetu)
* monitorowanie i przechwytywanie sygnałów przychodzących do targetu
* pisanie i czytanie pamięci oraz stanu rejestrów targetu
* zatrzymywanie i wznawianie pracy targetu, w kilku trybach:
  * `PTRACE_CONT`: target jest wznawiany do otrzymania sygnału lub ręcznego zatrzymania
  * `PTRACE_SYSCALL`: jak wyżej, ale target jest zatrzymywany przed wejściem do syscalla oraz po wyjściu z niego
  * `PTRACE_SINGLESTEP`: target jest wznawiany, wykonuje jedną instrukcję procesora, po czym jest zatrzymywany
  * `PTRACE_SYSEMU`, `PTRACE_SYSEMU_SINGLESTEP`: jak `PTRACE_SYSCALL` i `PTRACE_SINGLESTEP`, ale syscalle wywoływane
    przez target nie są wykonywane przez kernel

Niestety, mechanizm ten jest bardzo prymitywny, i nie pozwala na wykonywanie wielu operacji bardzo przydatnych przy pisaniu debuggerów. Jednym z przykładów jest manipulacja otwartymi plikami czy mapowaniami w procesie (np. nie da się utworzyć nowego mapowania). Aby obejść to ograniczenie, debuggery muszą:

* znaleźć jakieś miejsce w segmencie kodu targetu
* zachować jego zawartość
* wstrzyknąć w to miejsce instrukcję wywołującą syscall
* zachować obecny stan rejestrów
* wstawić do rejestrów numer i parametry syscalla robiącego to, co debugger chce (np. mmap)
* ustawić wskaźnik instrukcji tak, aby wskazywał na wstrzyknięty kod
* uruchomić `PTRACE_SINGLESTEP`
* wyciągnąć wynik syscalla z rejestru
* odtworzyć stan rejestrów
* odtworzyć nadpisane miejsce w segmencie kodu

Mechanizm ten ma pewne wady.

## Zadanie

Dopisać do syscalla `ptrace` operacje pozwalające na manipulację otwartymi plikami i mapowaniami innych procesów:

* `PTRACE_REMOTE_MMAP`: tworzy nowe mapowanie w zdalnym procesie. addr jest ignorowane, data wskazuje na strukturę 
  `ptrace_remote_mmap`, zawierającą parametry do wywołania:
  *  `addr`: adres pod którym ma być utworzone mapowanie, bądź 0 aby jądro automatycznie wybrało adres (jak w zwykłym mmap).
    Po udanym wywołaniu, jądro zapisze w tym polu wybrany adres.
  * `length`: jak w zwykłym mmap.
  * `prot`: jak w zwykłym mmap.
  * `flags`: jak w zwykłym mmap.
  * `fd`: deskryptor pliku, który ma być zmapowany. Deskryptor pliku jest szukany w plikach procesu wywołującego `ptrace`,
    a nie procesu w którym będzie tworzone mapowanie. Jeśli flags zawiera MAP_ANONYMOUS, to pole jest ignorowane.
  * `offset`: jak w zwykłym mmap.
  
  Jeśli tworzenie mapowania się udało, wynikiem tego wywołania jest 0, a adres mapowania jest zwracany w polu `addr`
  przekazanej struktury. Jeśli się nie udało, zwracany jest kod błędu, a pole addr nie jest modyfikowane.
* `PTRACE_REMOTE_MUNMAP`: usuwa mapowanie w zdalnym procesie. `addr` jest ignorowane, a `data` wskazuje na strukturę
  `ptrace_remote_munmap`, zawierającą parametry wywołania. Wartość zwracana i parametry są takie, jak przy zwykłym munmap.
* `PTRACE_REMOTE_MREMAP`: jak wyżej, ale dla mremap.
* `PTRACE_REMOTE_MPROTECT`: jak wyżej, ale dla mprotect.
* `PTRACE_DUP_TO_REMOTE`: zachowuje się jak dup, ale tworzy nowy deskryptor w zdalnym procesie. `addr` jest ignorowany,
  a `data` wskazuje na strukturę `ptrace_dup_to_remote`:
  * `local_fd`: deskryptor pliku w procesie wywołującym ptrace, który chcemy zduplikować do procesu śledzonego
  * `flags`: flagi nowego deskryptora. Jedyną zdefiniowaną flagą jest O_CLOEXEC.
  
  Wynikiem wywołania jest deskryptor utworzony w zdalnym procesie, lub kod błędu.
* `PTRACE_DUP2_TO_REMOTE`: zachowuje się jak dup3, ale tworzy nowy deskryptor w zdalnym procesie. Jak wyżej,
  ale pobiera dodatkowy parametr:
  * `remote_fd`: deskryptor pliku, który ma być utworzony (bądź zamieniony) w zdalnym procesie.
* `PTRACE_DUP_FROM_REMOTE`: zachowuje się jak dup, ale duplikuje deskryptor ze zdalnego procesu do procesu wywołującego 
  `ptrace`. `data` wskazuje na strukturę `ptrace_dup_from_remote`:
  * `remote_fd`: zdalny deskryptor pliku do zduplikowania.
  * `flags`: jak w `PTRACE_DUP_TO_REMOTE`.

  Wynikiem wywołania jest nowy deskryptor w procesie wywołującym, bądź kod błędu.
* `PTRACE_REMOTE_CLOSE`: zamyka deskryptor pliku w zdalnym procesie. data wskazuje na strukturę `ptrace_remote_close`, 
  zawierającą zdalny deskryptor. Wynik wywołania powinien być taki, jak wynik wywołania close.

Powyższe wywołania powinny działać na procesie, który jest przez nas śledzony i jest akurat zatrzymany przez ptrace. W przypadku użycia na innym procesie, należy zwrócić `ESRCH`.
