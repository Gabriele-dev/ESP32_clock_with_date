// Progetto gestione orario con regolazione WiFi su base giornaliera 24H e gestione data
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>         
#include <LiquidCrystal_I2C.h>

WebServer server(80);
WiFiManager wm;

LiquidCrystal_I2C lcd2004(0x27, 20, 4);

ulong scartoOrarioMillis = 0;
ulong scartoGiorni = 0;
uint8_t fineMese = 31;
int fineAnno = 365;
uint8_t gg = 1;
uint8_t im = 0;
int igSett = 4;  // Venerdì 1 Gennaio 2021
int anno = 2021;
String ggSett[7] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
String mese[12] = {"Gennaio  ", "Febbraio ", "Marzo    ", "Aprile   ", "Maggio   ", "Giugno   ", "Luglio   ", "Agosto   ", "Settembre", "Ottobre  ", "Novembre ", "Dicembre "};
int ore;
int minuti;
int secondi;
const uint8_t inPinSetup = 14;  //d14 gpio switch ground to setup Wi-Fi
const uint8_t inPinLight = 12;  //d12 gpio button groung to turn on display backlight
String constr_ora(int Ore, int Minuti, int Secondi) {
  String res = "";
  if (Ore<10){
    res.concat("0");
  }
  res.concat(String(Ore)+":");
  if (Minuti<10){
    res.concat("0");
  }
  res.concat(String(Minuti)+":");
  if (Secondi<10){
    res.concat("0");
  }
  res.concat(String(Secondi));
  return res;
}
String constr_data(int anno, String mese, uint8_t giorno, String ggSettimana){
  String res = "";
  res.concat(ggSettimana + " ");
  if (giorno<10) {
    res.concat(" ");
  }
  res.concat(String(giorno) + " ");
  res.concat(mese + " ");
  anno = anno % 100;
  res.concat(String(anno));
  return res;
}
String constr_html() {
  String HTML;
  getOrario(scartoOrarioMillis);
  HTML = "<!DOCTYPE html><html><body><h1>Welcome</h1><p>Your first Iot Project made with ESP32</p><p>&#128522;</p></br><table><tr><td><form action=\"/HP\" method=\"POST\"><input type=\"submit\" value=\"Hours up\"></form></td><td><form action=\"/MP\" method=\"POST\"><input type=\"submit\" value=\"Minutes up\"></form></td><td><form action=\"/SP\" method=\"POST\"><input type=\"submit\" value=\"Seconds\"></form></td></tr>";
  HTML.concat("<tr><td>"+String(ore)+"</td><td>"+String(minuti)+"</td><td>"+String(secondi)+"</td></tr>");
  HTML.concat("<tr><td><form action=\"/HL\" method=\"POST\"><input type=\"submit\" value=\"Hour less\"></td><td></form><form action=\"/ML\" method=\"POST\"><input type=\"submit\" value=\"Minutes less\"></form></td><td></td></tr>");
  HTML.concat("</table><br></br><table>");
  HTML.concat("<tr><td><form action=\"/YEP\" method=\"POST\"><input type=\"submit\" value=\"Year up\"></form></td><td><form action=\"/MOP\" method=\"POST\"><input type=\"submit\" value=\"Month up\"></form></td><td><form action=\"/DAP\" method=\"POST\"><input type=\"submit\" value=\"Day up\"></form></td></tr>");
  HTML.concat("<tr><td>"+String(anno)+"</td><td>"+String(mese[im])+"</td><td>"+String(gg)+"</td><td>"+ggSett[igSett]+"</td></tr>");
  HTML.concat("<tr><td><form action=\"/YEL\" method=\"POST\"><input type=\"submit\" value=\"Year less\"></form></td><td><form action=\"/MOL\" method=\"POST\"><input type=\"submit\" value=\"Month less\"></form></td><td><form action=\"/DAL\" method=\"POST\"><input type=\"submit\" value=\"Day less\"></form></td></tr>");
  HTML.concat("</table></body></html>");
  return HTML;
}

void cambioData() {
  igSett = (igSett + 1)%7;
  gg++;
  if (gg>fineMese) {
    gg=1;
    im++;
    if (im>11) {
      anno++;
      im = 0;
      if ((anno%4)==0) {
        fineAnno = 366;
      } else {
        fineAnno = 365;
      }
    }
    switch (im) {
      case 0:
      case 2:
      case 4:
      case 6:
      case 7:
      case 9:
      case 11: {
        fineMese = 31;
      }
      break;
      case 1: {
        if ((anno%4)==0) {
          fineMese = 29;
        } else {
          fineMese = 28;
        }
      }
      break;
      case 3:
      case 5:
      case 8:
      case 10: {
        fineMese = 30;
      }
      break;
    }
  }
}
  
void decrementoData(bool togliDay, bool togliMonth, bool togliYear) {
  if (togliDay) {
    gg--;
    igSett=(7+igSett-1)%7;
    if (gg==0) {
      gg=fineMese;
      igSett = (igSett+fineMese)%7;
    }
  }
  if (togliMonth) {
    bool calcEseguito = false;
    if (im==0){
      im += 12;
      igSett = (42 + igSett + fineAnno - 31)%7;
      calcEseguito = true;
    }
    im--;
    switch (im) {
      case 0:
      case 2:
      case 4:
      case 6:
      case 7:
      case 9:
      case 11: {
        fineMese = 31;
      }
      break;
      case 1: {
        if ((anno%4) == 0) {
          fineMese = 29;
        } else {
          fineMese = 28;
        }
      }
      break;
      case 3:
      case 5:
      case 8:
      case 10: {
        fineMese = 30;
      }
      break;
    }
    if (gg>fineMese) {
      igSett = (42+igSett-(gg-fineMese)-fineMese)%7;
      gg=fineMese;
    } else {
      if (!calcEseguito) {
        igSett = (35+igSett-(fineMese%7))%7;
      }
    }
  }
  if (togliYear) {
    int annoOld = anno;
    int fineAnnoOld = fineAnno;
    anno--;
    if (((annoOld%4)==0)||((anno%4)==0)) {
      if (im==1) {
        if ((anno%4)==0) {
          fineMese=29;
        } else {
          fineMese=28;
        }
      }
      if ((anno%4)==0) {
        fineAnno=366;
      } else {
        fineAnno=365;
      }
    }
    if (((im<1)||(im==1)&&(gg<29))&&((anno%4)==0)) {
      igSett = (371+igSett-fineAnno)%7;
    } else if ((im>1)&&((anno%4)==0)) {
      igSett = (371+igSett-(fineAnnoOld%7))%7;  
    } else if (((im>1)||(im==1)&&(gg==29))&&((anno%4)!=0)) {
      if ((im==1)&&(gg==29)) {
        gg--;
      }
      igSett = (371 + igSett - fineAnnoOld)%7;
    } else {
      igSett = (371+igSett-fineAnno)%7;  // caso in cui anno bisestile o no la data sia antecedente al 29 Febbraio
    }
  }
}

void incrementoData(bool aggiungiDay, bool aggiungiMonth, bool aggiungiYear) {
  if (aggiungiDay) {
    gg++;
    igSett++;
    if (gg>fineMese) {
      gg -= fineMese; 
      igSett = (35+igSett-fineMese)%7;
    } else {
      if (igSett>6) {
        igSett = igSett%7;
      }
    }    
  }
  if (aggiungiMonth) {
    im++;
    if (im>11) {
      im = im%12; 
      igSett = (371+31+igSett-fineAnno)%7;
    } else {
      igSett = (igSett + fineMese)%7;
    }
  }
  if (aggiungiYear) {
    int fineAnnoOld = fineAnno;
    int annoOld = anno;
    anno++;
    if ((anno%4)==0) {
      fineAnno = 366;
    } else {
      fineAnno = 365;
    }
    if ((im<=1)) {
      igSett = (igSett+fineAnnoOld)%7;
      if ((im==1)&&(gg==29)) {
        gg--;
        igSett = (igSett + 6)%7;
      }
    } else {
      igSett = (igSett+fineAnno)%7;
    }
  }
  switch (im) {
    case 0:
    case 2:
    case 4:
    case 6:
    case 7:
    case 9:
    case 11: { // Gennaio, Marzo, Maggio, Luglio, Agosto, Ottobre, Dicembre giorni 31
      fineMese = 31;
      if (gg>fineMese) {
        igSett = (7 + igSett - (gg-fineMese))%7;
        gg = fineMese;
      }
    }
    break;
    case 1: { // Febbraio
      if ((anno%4)==0) {
        fineMese = 29;
      } else {
        fineMese = 28;
      }
      if (gg>fineMese) {
        igSett = (7 + igSett - (gg-fineMese))%7;
        gg = fineMese;
      }
    }
    break;
    case 3:
    case 5:
    case 8:
    case 10: {  // Aprile, Giugno, Settembre, Novembre giorni 30
      fineMese = 30;
      if (gg>fineMese) {
        igSett = (7 + igSett - (gg-fineMese))%7;
        gg = fineMese;
      }
    }
    break;
  }
}

void getOrario(ulong orarioMillisec) {
  ulong millisToAdd = millis();
  orarioMillisec += millisToAdd;
  if (trunc(orarioMillisec/86400000)>scartoGiorni){
    cambioData();
    scartoGiorni++;
  }
  orarioMillisec = orarioMillisec%86400000;
  ore = (orarioMillisec/3600000)%24;
  minuti = (orarioMillisec/60000)%60;
  secondi = (orarioMillisec/1000)%60;
}

void setOrario(int Ore, int Minuti, int Secondi) {
  ulong millisToSubtract;
  ulong orarioMillis = Ore*3600000 + Minuti*60000 + Secondi*1000 + 86400000;
  millisToSubtract = (millis()%86400000);
  scartoOrarioMillis = (orarioMillis - millisToSubtract) % 86400000;
}

void handle_HP() {
  //   Aggiungere 1 ora all'orario * *
  ore++;
  scartoOrarioMillis += 3600000;
  scartoOrarioMillis = scartoOrarioMillis%86400000;
  if (ore>23) {
    ore = ore%24;
  }
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_MP() {
  //   Aggiungere 1 minuto all'orario * *
  minuti++;
  scartoOrarioMillis += 60000;
  scartoOrarioMillis = scartoOrarioMillis%86400000;
  if (minuti>59) {
    ore++;
    if (ore>23) {
      ore = ore%24;
    }
    minuti = minuti%60;
  }
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_SP() {
  //   Azzerare secondi dell'orario (arrotondando i minuti)  
  getOrario(scartoOrarioMillis);
  if (secondi<30) {
    secondi = 0;
  } else {
    secondi = 0;
    minuti++;
    if (minuti>59) {
      ore++;
      if (ore>23) {
        ore = ore%24;
      }
      minuti = minuti%60;
    }
  }
  setOrario(ore, minuti, secondi);
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_HL() {
  //   Togliere 1 ora all'orario  
  ore--;
  if (ore<0) {
    ore = ore + 24;
  }
  setOrario(ore, minuti, secondi);
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_ML() {
  //   Togliere 1 minuto all'orario  
  minuti--;
  if (minuti<0) {
    ore--;
    if (ore<0) {
      ore = ore + 24;
    }
    minuti = minuti + 60;
  }
  setOrario(ore, minuti, secondi);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_YEP() {
  // Incremento anno
  incrementoData(false, false, true);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_MOP() {
  // Incremento mese
  incrementoData(false, true, false);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_DAP() {
  // Incremento giorno
  incrementoData(true, false, false);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_YEL() {
  // Decremento anno
  decrementoData(false, false, true);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_MOL() {
  // Decremento Mese
  decrementoData(false, true, false);
  server.sendHeader("Location","/");
  server.send(303);
}
void handle_DAL() {
  // Decremento giorno
  decrementoData(true, false, false);
  server.sendHeader("Location","/");
  server.send(303);
}

void handle_root() {
  server.send(200, "text/html", constr_html());
}

void connessione() {

  wm.autoConnect("Broadcast", "Password");  // Change your Broadcast name and password please
  
  server.on("/", handle_root);
  server.on("/HP", handle_HP);
  server.on("/MP", handle_MP);
  server.on("/SP", handle_SP);
  server.on("/HL", handle_HL);
  server.on("/ML", handle_ML);
  server.on("/YEP", handle_YEP);
  server.on("/MOP", handle_MOP);
  server.on("/DAP", handle_DAP);
  server.on("/YEL", handle_YEL);
  server.on("/MOL", handle_MOL);
  server.on("/DAL", handle_DAL);
  server.begin();
  
  IPAddress ip;
  
  lcd2004.clear();
  lcd2004.setCursor(0, 0);
  lcd2004.print("Server started");
  delay(100); 
  lcd2004.setCursor(0, 1);
  lcd2004.print("Wi-Fi Connected");
  lcd2004.setCursor(0, 2);
  ip = WiFi.localIP();
  String indi = "";
  indi.concat(String(ip[0])+".");
  indi.concat(String(ip[1])+".");
  indi.concat(String(ip[2])+".");
  indi.concat(String(ip[3]));
  lcd2004.print("IP:"+indi);

  while (digitalRead(inPinSetup)==LOW) {
    server.handleClient();
  }
 
  wm.disconnect();
  lcd2004.init();
  lcd2004.setBacklight(HIGH);
  lcd2004.clear();
  
}

void setup() {

  pinMode(inPinSetup, INPUT_PULLUP);
  pinMode(inPinLight, INPUT_PULLUP);
  
  lcd2004.begin(20, 4);
  lcd2004.init();
  lcd2004.setBacklight(HIGH);
  lcd2004.clear();
  lcd2004.setCursor(0, 0);
   
}

void loop() {
  
  while (digitalRead(inPinSetup)==LOW) {
    lcd2004.init();
    lcd2004.setBacklight(HIGH);
    lcd2004.clear();
    lcd2004.setCursor(0, 0);
    getOrario(scartoOrarioMillis);
    connessione();
    
  }
  
  getOrario(scartoOrarioMillis);
  
  if (digitalRead(inPinLight)==LOW) {  
    lcd2004.init();
    lcd2004.setBacklight(HIGH);
    lcd2004.clear();
  } 
  lcd2004.setCursor(0, 0);   
  lcd2004.print(constr_ora(ore, minuti, secondi));
  lcd2004.setCursor(0, 1);
  lcd2004.print(constr_data(anno, mese[im], gg, ggSett[igSett]));
      
 

}
