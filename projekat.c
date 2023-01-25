#include "display.c"

//Serijska komunikacija
char * ok = "OK\r\n";
char * greska = "ERROR\r\n";
char * pokZaSlanje;
char buffer[10];
char buffer_it = 0;

//Timer
int brojacPrekida = 0;
unsigned char sekunda = 0;

//ON-OF
unsigned char start = 0;

//Izbor mod-a
unsigned char mod = 1; // 1-Brojanje nitni, 2-Pomeranje gramofona
unsigned char modEn = 0; // Prvi mod nece raditi dok mu ne damo odgovarajuci broj nitni za brojanje
unsigned char flag = 0; //Flag koji nam govori da li je i jedan mod izvrsen od starta uredjaja

//Broj nitne
unsigned char nitneZaPomeraj = 0;
unsigned char trenutniPomeraj = 0;

//Ulazi sa gramofona
unsigned char brojacZaTaster1 = 0; //Naisao na nitnu
unsigned char trenutno_hardversko_stanje_P0_0 = 1;
unsigned char prethodno_hardversko_stanje_P0_0 = 1;
unsigned char trenutno_softversko_P0_0 = 1;

unsigned char brojacZaTaster2 = 0; //Napravio pun krug
unsigned char trenutno_hardversko_stanje_P0_1 = 1;
unsigned char prethodno_hardversko_stanje_P0_1 = 1;
unsigned char trenutno_softversko_P0_1 = 1;

void timer1_interrupt(void) interrupt 3 {
  TL1 = 0x48;
  TH1 = 0x48;

  if (++brojacPrekida == 5000) {
    if (start) {
      sekunda++;
      if (sekunda == 60) {
        sekunda = 0;
      }
    }
    brojacPrekida = 0;
  }

  //Debounce za ulaz sa gramofona
  trenutno_hardversko_stanje_P0_0 = P0_0;
  if (trenutno_hardversko_stanje_P0_0 == prethodno_hardversko_stanje_P0_0) {
    if (++brojacZaTaster1 == 5) {
      trenutno_softversko_P0_0 = trenutno_hardversko_stanje_P0_0;
      brojacZaTaster1 = 0;
    }
  } else {
    brojacZaTaster1 = 0;
  }
  prethodno_hardversko_stanje_P0_0 = trenutno_hardversko_stanje_P0_0;

  trenutno_hardversko_stanje_P0_1 = P0_1;
  if (trenutno_hardversko_stanje_P0_1 == prethodno_hardversko_stanje_P0_1) {
    if (++brojacZaTaster2 == 5) {
      trenutno_softversko_P0_1 = trenutno_hardversko_stanje_P0_1;
      brojacZaTaster2 = 0;
    }
  } else {
    brojacZaTaster2 = 0;
  }
  prethodno_hardversko_stanje_P0_1 = trenutno_hardversko_stanje_P0_1;

}

void parsiraj_poruku() {
  if (buffer[0] == '(' && buffer[1] == 'S' && buffer[2] == 'T' && buffer[3] == 'A' && buffer[4] == 'R' && buffer[5] == 'T' && buffer[6] == ')') {
    start = 1;
    pokZaSlanje = ok;
    flag = 0;
  } else if (buffer[0] == '(' && buffer[1] == 'S' && buffer[2] == 'T' && buffer[3] == 'O' && buffer[4] == 'P' && buffer[5] == ')') {
    start = 0;
    pokZaSlanje = ok;
    mod = 1;
    sekunda = 0;
    trenutniPomeraj = 0;
    modEn = 0;
    nitneZaPomeraj = 0;
    flag = 0;
  } else if (buffer[0] == '(' && buffer[1] == 'M' && buffer[2] == ':' && buffer[4] == ')') {
    if (buffer[3] == '1') {
      pokZaSlanje = ok;
      mod = 1;
      sekunda = 0;
      trenutniPomeraj = 0;
      nitneZaPomeraj = 0;
      flag = 0;
    } else if (buffer[3] == '2') {
      pokZaSlanje = ok;
      mod = 2;
      modEn = 0;
    } else {
      pokZaSlanje = greska;
    }
  } else if ((buffer[0] == '(' && buffer[1] == 'B' && buffer[2] == ':' && buffer[4] == ')') && (mod == 2)) {
    sekunda = 0;
    trenutniPomeraj = 0;
    nitneZaPomeraj = buffer[3] -= 48;
    pokZaSlanje = ok;
    modEn = 1;
    flag = 0;
  } else {
    pokZaSlanje = greska;
  }
  SBUF = * pokZaSlanje;
  buffer[0] = '\0';
  buffer_it = 0;
}

void serijski_prekid(void) interrupt 4 {
  if (RI) {
    char prijem;
    RI = 0;
    prijem = SBUF;

    if (prijem == '(') {
      buffer_it = 0;
    }

    buffer[buffer_it] = prijem;
    buffer_it = (buffer_it + 1) % 10;

    if (prijem == ')') {
      parsiraj_poruku();
    }
  }
  if (TI) {
    TI = 0;

    pokZaSlanje++;
    if ( * pokZaSlanje != '\0') {
      SBUF = * pokZaSlanje;
    }
  }
}

char * num2str(int broj) {
  unsigned int i = 0;
  unsigned int j;
  unsigned int ostatak;
  unsigned int len = 0;
  unsigned int lenstr = 0;
  char str[8];
  char pom[8];
  while (broj != 0) {
    ostatak = broj % 10;
    broj = broj / 10;
    pom[i] = ostatak + 48;
    len++;
    i++;
  }
  pom[len] = '\0';
  lenstr = len;
  j = len - 1;
  for (i = 0; i < lenstr; i++, j--) {
    str[i] = pom[j];
  }
  str[lenstr] = '\0';
  return str;
}

void main(void) {

  //Ulaz sa gramofona
  unsigned char trenutno_stanje_P0_0 = 1;
  unsigned char prethodno_stanje_P0_0 = 1;
  unsigned char trenutno_stanje_P0_1 = 1;
  unsigned char prethodno_stanje_P0_1 = 1;

  //Tajmer 0 
  TL1 = 0x48;
  TH1 = 0x48;
  TMOD = 0x20;
  TR1 = 1;
  ET1 = 1;

  //Serijska komunikacija
  PCON &= 0x80;
  BRL = 253;
  SCON = 0x50;
  BDRCON |= 0x1C;

  ES = 1;
  EA = 1;

  start = 0;
  mod = 1;

  initDisplay();
  while (1) {

    if (start) {
      //Rising edge za ulaz sa gramofona
	  P2_0 = 0;
      trenutno_stanje_P0_0 = trenutno_softversko_P0_0; //Naisao na nitnu
      if (trenutno_stanje_P0_0 > prethodno_stanje_P0_0) {
        trenutniPomeraj += 1;
      }
      if ((mod == 2) && (modEn == 1)) {
        if (flag == 0) {
          clearDisplay();
          writeLine("MOD 2");
        }

        if (trenutniPomeraj == nitneZaPomeraj) {
          //Ispis vremena za koje se uredjaj pomerao
          clearDisplay();
          //writeLine("MOD 2");
          writeLine("STOP2");
          newLine();
          writeLine(num2str(sekunda));
          trenutniPomeraj = 0;
          sekunda = 0;
          flag = 1;
          start = 0;
          mod = 1;
          modEn = 0;
          nitneZaPomeraj = 0;

        }
      }

      trenutno_stanje_P0_1 = trenutno_softversko_P0_1; //Napravio pun krug
      if (mod == 1) {
        if (flag == 0) {
          clearDisplay();
          writeLine("MOD 1");
        }
        if (trenutno_stanje_P0_1 > prethodno_stanje_P0_1) {
          //Ispis broja nitni i vremena posle kruga motora
          clearDisplay();
          //writeLine("MOD 1");
          writeLine("STOP1");
          writeLine(num2str(trenutniPomeraj));
          newLine();
          writeLine(num2str(sekunda));
          trenutniPomeraj = 0;
          sekunda = 0;
          flag = 1;
          start = 0;
          mod = 1;
          modEn = 0;
          nitneZaPomeraj = 0;
        }
      }

    } else {
	  P2_0 = 1;
      sekunda = 0;
	  if(flag == 0)
	  {
      	clearDisplay();
      	writeLine("STOP");
	  }
    }

    prethodno_stanje_P0_0 = trenutno_stanje_P0_0;
    prethodno_stanje_P0_1 = trenutno_stanje_P0_1;

  }

}