W rozwiązaniu wykorzystałem istniejące funkcje w kodzie jądra, aby na ich
podstawie stworzyć ich bardziej ogólne kopie (np. przyjmujące parametr
`struct task_struct`, dla którego należy wykonać mapowanie zamiast używać
`current`). Tak rekurecyjnie dla każdej wywoływanej w nich funkcji, która
zakładała wykonanie na aktualnym procesie.

Aby uniknąć nadmiaru kodu, zachowując jednak stare funkcje używane w wielu
miejscach kodu kernela, jeżeli tylko było to możliwe istniejące wcześniej
funkcje zmienialem aby jedynie wywoływały bardziej ogólną wersje z parametrem
current.
