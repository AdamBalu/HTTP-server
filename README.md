|**PROJEKT** | **BODY** |
|:-----------|:---------|
| IPK1       | 20/20    |

# IPK - Projekt 1 - HTTP server

## Popis
Server v jazyku C/C++ komunikujúci prosredníctvom protokolu HTTP, ktorý poskytuje rôzne informácie o systéme. Server si vytvorí socket, naslúcha na zadanom porte a podľa URL vracia požadované informácie.

Použité knižnice (okrem C stdlib):
```c
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
```
## Použitie
```
$ ./hinfosvc PORT
```
**PORT** - číslo portu, ktorým user pripája na server
## Príklady spustenia
```
$ ./hinfosvc 12345
$ ./hinfosvc 5000
$ ./hinfosvc 9999
```
Po pripojení na socket pomocou daného **portu** (server naslúcha a čaká na request) je možné podať žiadosť na tento server. Server spracováva žiadosť GET, odpovedá na žiadosti load, cpu-name, hostname. Je niekoľko alternatív použitia:
```
$ GET http://servername:PORT/[load|cpu-name|hostname]
$ wget http://servername:PORT/[load|cpu-name|hostname]
$ curl http://servername:PORT/[load|cpu-name|hostname]
```
**wget** vygeneruje súbor **load|cpu-name|hostname** obsahujúci server response

**curl** produkuje server response v danom termináli
<br />
<br />

Ďalšou možnosťou je otvorenie lokality vo webovom prehliadači a nasledujúce zadanie domény:
```
http://servername:PORT/[load|cpu-name|hostname]
```
Server response sa v tomto prípade zobrazí _priamo na stránke_

## Step-by-step návod na použitie
**Terminál č. 1**
```
$ ./hinfosvc 5000

```
_...server očakáva request_

**Terminál č. 2**
```
$ curl http://localhost:5000/cpu-name
AMD Ryzen 7 1700X Eight-Core Processor
$ curl http://localhost:5000/load
2.65%
$ GET http://localhost:5000/hostname
jozko-mrkvicka-MS-3493
$
```
**Terminál č. 1**
```
$ ./hinfosvc 5000
^C
$
```
_Pripojenie k serveru je týmto ukončené (CTRL + C)_

**Terminál č. 2**
```
$ GET http://localhost:5000/load
Can't connect to localhost:5000 (Connection refused)

Connection refused at /usr/../http.pm line 50.
```
