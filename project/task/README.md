# POP3 server

## Popis

**Vedie**:

[Ing. Martin Holkovič](http://www.fit.vutbr.cz/~iholkovic/)

**Popis varianty**:

Vytvořte komunikující aplikaci podle konkrétní vybrané specifikace pomocí síťové knihovny BSD sockets. Projekt bude vypracován v jazyce C/C++. Vytvorte program popser, ktorý bude plniť úlohu POP3 [1] serveru (ďalej iba server). Na server sa budú pripájať klienti, ktorý si pomocou protokolu POP3 sťahujú dáta zo serveru. Server bude pracovať s emailmi uloženými vo formáte IMF [4] v adresárovej štruktúre typu Maildir [2].

Vašim cieľom je teda vytvoriť program, ktorý bude vedieť pracovať s emilami uloženými v adresárovej štruktúre Maildir. Program bude pracovať iba s jednou poštovou schránkou. Takto uložené emaily budú prostredníctvom protokolu POP3 poskytované jedinému používateľovi.

**Vyžadované vlastnosti**:

* Program využíva iba hlavičkové súbory pro prácu so soketmi a ďalšie funkcie používané v sieťovom prostredí (ako je netinet/*, sys/*, arpa/* apod.), knižnicu pre prácu s vláknami (pthread), signálmi, časom, rovnako ako štandardnú knižnicu jazyka C (varianty ISO/ANSI i POSIX), C++ a STL. Ostatné knižnice nie sú povolené.
* Program nespúšťa žiadne ďalšie procesy.
* Pre prácu s emailom sa musí používateľ vždy autentizovať.
* Server musí využívať paralelný a neblokujúci soket.
* Preklad sa spustí príkazom "make", ktorý vytvorí binárku s názvom "popser".
* V odovzdanom archíve sa nenachádzajú žiadne adresáre.
* Povolené jazyky pre dokumentáciu, readme a komentáre zdrojových kódov sú slovenčina, čeština a angličtina.
* Všetky súbory, ktoré sú potrebné k behu programu sa musia nachádzať v rovnakej zložke ako binárka. Používateľ, ktorý daný proces spúšťa bude mať vždy právo na zápis+čítanie z danej zložky.
* Okrem parametru -h by program na stdout nemal vypisovať vôbec nič. Všetky chybové (napr. port obsadený) a ladiace výpisy vypisujte na stderr.
* V prípade, že nastane ľubovoľná chyba (napr. port obsadený, zlé parametre), je potrebné na STDERR vypísať chybovú hlášku popisujúcu problém (text typu "Nastala nejaká chyba" je nedostačujúci). Nie je vyžadovaný žiadny striktný formát, je len potrebné aby používateľ z textu pochopil problém.
* Poradie parametrov nie je pevne určené.
* Na danej stanici beží vždy len 1 proces pracujúci s danou Maildir zložkou. Takže nie je nutné riešiť vyhradený prístup k danej zložke.
* Je potrebné implementovať celú špecifikáciu POP3 (povinné, odporúčané aj voliteľné časti [3]) [1] vrátane sekcie č.13, okrem:
    * implementácie šifrovania (TLS/SSL),
    * sekcie č.8, ktorá obsahuje "úvahy" (norma totiž nevyžaduje ich implementovanie),
    * príkaz TOP, ktorý bude braný iba ako rozšírenie projektu. 
* NEimplementujte žiadne ďalšie rozšírenia ani "triky" z protokolu IMAP.
* S výnimkou príkazov TOP a RETR je možné čítať obsah súborov s emailmi maximálne 1x a to pri presune emailu zo zložky /new/ do /cur/ v rámci adresárovej štruktúry Maildiru.
* Server beží až kým neprijme signál SIGINT. Vtedy musí svoj beh správne a bezpečne ukončiť.
* Je potrebné počítať s tým, že proces serveru môže byť ukončený a následne znovu spustený.
* Do adresárovej štruktúry Maildir sa nevytvárajú žiadne nové súbory ani zložky. Jediná vykonávaná činnosť je presunutie/premenovanie už existujúcich súborov.
* Pracujte len s IPv4.

**Súbor s prihlasovacími údajmi**:

Konfiguračný súbor s prihlasovacími údajmi bude obsahovat meno a heslo v jednoduchom formáte (dodržujte konvencie pre textové súbory v prostredí UNIX/Linux):

```
username = meno
password = heslo
```

**Použitie**:
./popser [-h] [-a PATH] [-c] [-p PORT] [-d PATH] [-r]

* h (help) - voliteľný parameter, pri jeho zadaní sa vypíše nápoveda a program sa ukončí
* a (auth file) - cesta k súboru s prihlasovacími údajmi
* c (clear pass) - voliteľný parameter, pri zadaní server akceptuje autentizačnú metódu, ktorá prenáša heslo v nešifrovanej podobe (inak prijíma iba heslá v šifrovanej podobe - hash)
* p (port) - číslo portu na ktorom bude bežať server
* d (directory) - cesta do zložky Maildir (napr. ~/Maildir/)
* r (reset) - voliteľný parameter, server vymaže všetky svoje pomocné súbory a emaily z Maildir adresárovej štruktúry vráti do stavu, ako keby proces popser nebol nikdy spustený (netýka sa časových pečiatok, iba názvov a umiestnení súborov) (týka sa to len emailov, ktoré sa v adresárovej štruktúre nachádzajú).

**3 režimy behu**:

* výpis nápovedy - zadaný parameter "-h"
* iba reset - zadaný iba parameter "-r"
* bežný režím - zadané parametre "-a", "-p", "-d" a voliteľné parametre "-c" a "-r"

**Rozšírenie** (súčet bodov za projekt nemôže presiahnuť 20b):
* príkaz TOP


**Literatúra**:

1. https://tools.ietf.org/html/rfc1939
2. https://cr.yp.to/proto/maildir.html
3. https://tools.ietf.org/html/rfc2119
4. https://tools.ietf.org/html/rfc5322
