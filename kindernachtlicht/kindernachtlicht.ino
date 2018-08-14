#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Time.h>
#include <NtpClientLib.h>

#include "myTypes.h"

#define bluePin 14    //PRO D5
#define redPin 12     //PRO D6
#define greenPin 13   //PRO D7

//WLAN
const char* ssid = "";
const char* pass = "";
String sleep;
String compareSleep = "";
String wake;
String compareWake = "";
String beforeWake;
String compareBeforeWake;
String lampState;
String brightness;

String PAGE_form = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title></title>
  <meta name=viewport content="width=device-width, initial-scale=1">
  <style type="text/css">
    body{
      background-color: #000;
      font-family: Helvetica, Arial;
      margin: 0.1em
    }
    h1, h2, h3, h4, h5, p{
      margin: 0;
      padding: 0;
    }
    a{
      color: #000;
      font-weight: bold;
      
    }
    body > header{
      text-align: center;
      background-color: #d35400;
      padding: 0.25em 0;      
    }
    body > main{
      background-color: #ecf0f1;        
    }
    body > main > article{
      text-align: center;
      padding: 1em 1em;
      color: #FFF;      
      background-color: #16a085;
      box-shadow: 0px 10px 31px -2px rgba(0,0,0,0.5);
    }
    body > footer{
      text-align: center;
      background-color: #bdc3c7;
      padding: 0.5em 0;
    }
    form {
      padding: 2em 0;
    }
    section{      
      padding: 1.5em;
      background-color: #ecf0f1;
      display: flex;
        flex-wrap: wrap;
        align-items: center;
    }
    form > section > label{
      display: inline-block;
        flex: 1 0 5%;
        max-width: 40%;
        font-size:1.2em;
    }
    form > section > label + *{
      flex: 1 0 20%;
      max-width: 45%;
    }   
    form > section > input + button{
      margin-left:0.25em;
      height: 2.6em;      
    }
    form > section button[type=submit]{
      margin:1.5em auto 0;
      width: 100%;  
      background-color: #27ae60;
      font-size: 1.2em;
      font-weight: bold;
    }
    button, input, select{
      border: 1px solid black;      
      background-color: #ccc;
      height: 2.5em;
      font-family: Helvetica, Arial;
      font-size:1.2em;
    }

  </style>
</head>
<body>
<main>
  <article>
    <h3>Ole's Einschlaflampe</h3>     
  </article>
  <section>
    <p>Diese Lampe soll dem Kind erm&ouml;glichen zu erkennen, ob es Zeit zum schlafen oder Zeit zum aufstehen ist.<br /><a href="#hinweise">Hier</a> gibt es weitere Hinweise.</p>  
  </section>
  <form id="oleslampe" method="post" autocomplete="off" name="oleslampe" action"/"> 
    <section>
      <label for="sleep">Einschlafen</label>
      <input type="time" name="sleep" id="sleep" value="" pattern="[0-9]*">
      <button type="button" id="js_set_now">Jetzt</button>
    </section>
    <section>
      <label for="wake">Aufstehen</label>
      <input type="time" name="wake" id="wake" value="" pattern="[0-9]*">
    </section>
    <section>
      <label for="beforeWake">Warnung</label>
      <select id="beforeWake"  name="beforeWake">
        <option value="0">Aus</option>
        <option value="5">5 Minutes</option>
        <option value="10">10 Minutes</option>
        <option value="15">15 Minutes</option>
        <option value="20" selected="selected" >20 Minuten</option>
        <option value="30" >30 Minuten</option>
        <option value="40" >40 Minuten</option>
        <option value="50" >50 Minuten</option>
        <option value="60" >60 Minuten</option>
        <option value="90" >90 Minuten</option>
        <option value="120" >120 Minuten</option>
      </select>
    </section>
    <section>
      <button type="submit" form_id="conf">Jetzt einschlafen</button>
    </section>
  </form>
  <section>
    <h4 id="hinweise">Hinweise</h4>
    <p>Diese Lampe leuchtet ab einem gew&auml;hlten <i>Einschlafzeitpunkt</i> rot und wechselt zur gew&auml;hlten <i>Aufstehzeit</i> zu gr&uuml;n. Optional kann bis 90 Minuten vor der Aufstehzeit eine <i>Aufstehwarnung</i> eingestellt werden, die die Lampe die eingestellt Zeitspanne vor dem Aufstehzeitpunkt gelb leuchten l&auml;sst - nach dem Motto: "Nicht mehr lange dann ist Aufstehzeit"</p>
    <p>Hier kannst Du die Einschlaf- und Aufwachzeiten einstellen. Der "jetzt" Button w&auml;hlt die aktuelle Zeit als Einschlafzeit aus. Die Aufwachzeit ist standardmä&auml;&szlig;ig auf 5:20 gestellt, kann aber nach belieben ge&auml;ndert werden. Im Feld "Aufwachwarnung" kann eingestellt werdem, wieviele Minuten vor dem Aufwachzeitpunkt die Lampe gelb leutet. </p>
  </section>
</main>
<footer>1.0.0 Juni 2018</footer>
  <script>
    document.getElementById('js_set_now').addEventListener('click', function () {
      var now = new Date();      
      var h = now.getHours() < 10 ? "0" + now.getHours() : now.getHours();
      var m = now.getMinutes() < 10 ? "0" + now.getMinutes() : now.getMinutes();
      document.getElementById('sleep').value = h + ":" + m;
    });
           
    document.getElementById('sleep').value = {{SLEEP}};
    document.getElementById('wake').value = {{WAKE}};
    
  </script>
</body>
</html>
)=====";

ESP8266WebServer server(80);
boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event
int8_t timeZone = 1;
int8_t minutesTimeZone = 0;

void setup() {
  pinMode(redPin,OUTPUT);
  pinMode(greenPin,OUTPUT);
  pinMode(bluePin,OUTPUT);
  
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  if (MDNS.begin("oleslampe")) {
    Serial.println("MDNS responder started");
  }
  
  server.on ( "/", handleRoot);  
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  //startupLamp();
  //setupLamp();

  NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
  });

   NTP.begin ("pool.ntp.org", 1, true, minutesTimeZone);
   NTP.setInterval (63); 
   checkLamp(); 
}


void loop() {
  static int i = 0;
  static int last = 0;
  
  if (syncEventTriggered) {
      processSyncEvent (ntpEvent);
      syncEventTriggered = false;
  }

  if ((millis () - last) > 51000) {
        //Serial.println(millis() - last);
        last = millis ();
        Serial.print (i); Serial.print (" ");
        Serial.print (NTP.getTimeDateString ()); Serial.print (" Time: ");
        Serial.print (NTP.getLastNTPSync ()); Serial.print (" ");       
        Serial.print (NTP.isSummerTime () ? "Summer Time. " : "Winter Time. ");
        Serial.print ("WiFi is ");
        Serial.print (WiFi.isConnected () ? "connected" : "not connected"); Serial.print (". ");
        Serial.print ("Uptime: ");
        Serial.print (NTP.getUptimeString ()); Serial.print (" since ");
        Serial.println (NTP.getTimeDateString (NTP.getFirstSync ()).c_str ());
        
        checkLamp();
        i++;
    }
    delay (0);
  
  // put your main code here, to run repeatedly:
  server.handleClient();

}
/*
 * Lampenschaltung Check der Start und Endzeiten
 * V2
 */
void checkLamp(){
  Serial.println("Check Lamp - Sleep:"+compareSleep+" Wake: "+compareWake+" Compare before Wake: "+compareBeforeWake+" State: "+lampState);

  if(compareSleep == "" || compareWake == ""){
    setBlue(100);
  }else{    
    String now = getCompareNow();
      bool b = now >= compareSleep;
    //compareWake
    //compareSleep
    if(now >= compareSleep){       
      if(lampState == "aus" || lampState == "gruen" || lampState == "blau"){
        //=> lampe rot
        setRed(50);
      }   
      if(compareWake != "0" && now >= compareBeforeWake && lampState == "rot"){
        // => lampe gelb
        setYellow(50);
      }
    }
    if(now >= compareWake && (lampState == "rot" || lampState == "gelb" )){
      setGreen(10);
    }
  }
}

/*
 * Webserver index Request behandeln
 */
void handleRoot() {
 Serial.println("Handle Root...");
 if (server.hasArg("sleep")) {
    handleSubmit();
    Serial.println("Handle submit...");
 }
 String pageTemplate = parseTemplate(PAGE_form);
 server.send ( 200, "text/html", pageTemplate ); 
}

/*
 * Platzhalte in Templates ersetzen
 */
String parseTemplate(String pageTemplate){
  String repSleep = "''";
   if(sleep != ""){
     repSleep = "'"+sleep+"'";
   }
   String repWake = "'05:20'";
   if(wake != ""){    
      repWake = "'"+wake+"'";
   }
   pageTemplate.replace("{{SLEEP}}",repSleep );
   pageTemplate.replace("{{WAKE}}",repWake);
   return pageTemplate; 
}

/*
 * Post Daten entgegennehmen
 */
void handleSubmit()
{
  sleep = server.arg("sleep");
  wake = server.arg("wake");  
  beforeWake = server.arg("beforeWake").toInt();

  completeSleepWakeTimes();
  Serial.println("compareSleep: "+compareSleep+"  compareWake: "+compareWake+" compareBeforeWake: "+compareBeforeWake);
  setOff();
  checkLamp();
}

void completeSleepWakeTimes(){
  compareSleep = sleep;
  compareSleep.remove(compareSleep.indexOf(":"),1);
  
  compareWake = wake;
  compareWake.remove(compareWake.indexOf(":"),1);

  compareBeforeWake = getCompareBeforeWake();
  addDateToSleepWake();
}

String getCompareNow(){
  return year()+prefixNull(month())+prefixNull(day())+prefixNull(hour())+prefixNull(minute());
}
/*
 * Doenst Work calculation with times
 */
String getCompareBeforeWake(){
  
  int beforeWakeIntMilliSeconds = beforeWake.toInt() * 60000;

  String compareWakeHrs = compareWake.substring(0, 2);
  String compareWakeMin = compareWake.substring(2, 4);
   
  //compareWake
  int compareWakeHrsMilliSeconds = compareWakeHrs.toInt() * 3600000;
  int compareWakeMinMilliSeconds = compareWakeMin.toInt() * 60000;
  
  int compareWakeMilliSeconds = compareWakeHrsMilliSeconds + compareWakeMinMilliSeconds;
  
  int yellowStart = compareWakeMilliSeconds - beforeWakeIntMilliSeconds;
  
  int minutes = (yellowStart / 60000) % 60;  
  int hrs = (yellowStart / 3600000) % 24;
  
  
  return prefixNull(hrs)+prefixNull(minutes);
}

void addDateToSleepWake(){
  String today = year()+prefixNull(month())+prefixNull(day());
  String tomorrow = year()+prefixNull(month())+prefixNull(day()+1);
  if(compareWake < compareSleep){
    compareWake = tomorrow+compareWake;
    compareBeforeWake = tomorrow+compareBeforeWake;
  }else{
    compareWake = today+compareWake;
    compareBeforeWake = today+compareBeforeWake;
  }
  compareSleep = today+compareSleep;
}

String prefixNull(int val){
  String prefix = "";
  if(val < 10){
    prefix = "0";
  }
  return prefix + String(val);
}


void setupLamp(){
  setOff();
  setBlue(100);
}

void setYellow(int b){
  if(lampState != "gelb"){
    analogWrite(redPin,1022/b);
    analogWrite(greenPin,400/b);
    analogWrite(bluePin,0/b);
    lampState = "gelb";
  }
}  
void setRed(int b){
  if(lampState != "rot"){
    analogWrite(redPin,1022/b);
    analogWrite(greenPin,0/b);
    analogWrite(bluePin,0/b);
    lampState = "rot";
  }
}
void setGreen(int b){
  if(lampState != "gruen"){
    analogWrite(redPin,0/b);
    analogWrite(greenPin,1022/b);
    analogWrite(bluePin,0/b);
    lampState = "gruen";
  }
}
void setBlue(int b){
  if(lampState != "blau"){
    analogWrite(redPin,0/b);
    analogWrite(greenPin,0/b);
    analogWrite(bluePin,1000/b);
    lampState = "blau";
  }
}
void setOff(){
  analogWrite(redPin,0);
  analogWrite(greenPin,0);
  analogWrite(bluePin,0);
  lampState = "aus";
}


void startupLamp(){
   setYellow(20);
  delay(500);
  setYellow(50);
  delay(500);
  setRed(20);
  delay(500);
  setRed(50);
  delay(500);
  setGreen(20);
  delay(500); 
  setGreen(50);
  delay(500); 
  setBlue(20);
  delay(500);
  setBlue(50);
  delay(1000); 
 }

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
        Serial.print ("Time Sync error: ");
        if (ntpEvent == noResponse)
            Serial.println ("NTP server not reachable");
        else if (ntpEvent == invalidAddress)
            Serial.println ("Invalid NTP server address");
    } else {
        Serial.println ("Got NTP time: ");
        Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
    }
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
