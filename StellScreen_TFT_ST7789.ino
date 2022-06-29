/*
  Projet StellScreen by "Astr' Au Dobson" astraudobson@gmail.com
  Code nécessitant une carte ESP32 + un écran TFT 240x320 ST7789 connecté conformément aux recommandations fournies avec ces 2 accessoires
*/

// Déclaration des bibliothèques
#include <Wire.h>                   
#include <WiFi.h>               
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include "Free_Fonts.h" // Include the header file attached to this sketch

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

// Déclaration des variables
String pays = "", site = "", longitude = "", latitude = "", date = "", time_zone = "", h_locale = "", str10 = "";
String az = "", alt="", obj_complet = "", obj_nom ="" ; String obj_cat = ""; String obj_cat_interm = ""; String obj_type = ""; 
String nom_ngc = "", nom_m = "", nom_IC, nom_etoile = "", nom_planete = "", obj_selected = "", info_supp = "";
String h_lever ="", h_coucher = "", h_culmi = "", magnitude = "", yj2000 = "", zj2000 = ""; int old_intzj2000 = 0, intzj2000 = 0;
String menu_principal[6] = { "Align with Sync.", "Action", "FOLLOW", "Object infos",  "Generales infos", "--- BACK ---" };
String menu_align_targets[5] = {"Deneb", "Vega", "Arcturus", "Mirach", "--- BACK ---"};
String menu_action[6] = {"Select. object", "Scope to obj.", "Scope to center", "Center selected obj.", "Select Scope", "--- BACK ---"};
bool sort = false, mode_follow = false, follow = false, circum = false;
int xpos =  0; int ypos = 0;

const char* ssid = "PC_PORTABLE";                                           // your SSID
const char* password = "123456789";                                         // SSID password
String serverName = "http://192.168.137.1:8090/";                           // example "http://192.168.1.6:8090/"
String ref_telescope = "1";                                                 // reférence télescope dans Stellarium
String requete =""; String httpRequestData = ""; int httpResponseCode = 0; String serverPath = "";

int menu_value = 0, old_menu_value=1, strate_menu = 0, compteur= 0, menu_targets_value = 0, menu_action_value = 0, menu_scope_value = 0;
int VALID = 13, SCROLL = 12;


// #######################################################################################################################################
//                                                            SETUP   /   LOOP
// #######################################################################################################################################
void setup() {
  initialisation();
  delay(1000);

     pinMode(SCROLL, INPUT_PULLUP);                                               // définition des pins des boutons
     pinMode(VALID, INPUT_PULLUP);
}

void loop() {
http_get();delay(50);
http_get_j2000();delay(50);
  
 //bouton de navigation dans les menus//
 if (digitalRead(SCROLL)== false){       
       if (strate_menu==0) {affiche_selected();}   
       if (strate_menu==1) {menu_value ++; if (menu_value == 6) {menu_value=0;}; draw_menu_principal();}                      // mettre à jour en cas d'ajout/supp d'options
       if (strate_menu==2) {menu_targets_value ++; if (menu_targets_value == 5) {menu_targets_value=0;} draw_menu_align_targets();}  // mettre à jour en cas d'ajout/supp d'options
       if (strate_menu==3) {menu_action_value ++; if (menu_action_value == 6) {menu_action_value=0;} draw_menu_action();}
       if (strate_menu==4) {menu_scope_value ++; if (menu_scope_value == 5) {menu_scope_value=0;} draw_menu_scope();}
  }

 //bouton de sélection//
 if (digitalRead(VALID)==false){
  if (mode_follow == true) {mode_follow = false;strate_menu=0; }
  if (strate_menu==0){tft.fillScreen(TFT_BLACK);  strate_menu=1; draw_menu_principal();}
  else if (strate_menu==1){                           // MAIN MENU
       if (menu_value == 0) {http_post_align_with_sync(); tft.fillScreen(TFT_BLACK); strate_menu=0;}
       if (menu_value == 1) {tft.fillScreen(TFT_BLACK); menu_action_value=0; strate_menu=3; draw_menu_action();}
       if (menu_value == 2) {mode_follow= true; menu_value=0; affiche_follow();}
       if (menu_value == 3) {affiche_infos_objet(); tft.fillScreen(TFT_BLACK); strate_menu=0;}
       if (menu_value == 4) {affiche_infos_generales(); tft.fillScreen(TFT_BLACK); strate_menu=0;} 
       if (menu_value == 5) {tft.fillScreen(TFT_BLACK); strate_menu=0;}           
  }
  else if (strate_menu==2) {                                 
    if(menu_align_targets[menu_targets_value]=="--- Back ---"){tft.fillScreen(TFT_BLACK);strate_menu = 1; draw_menu_principal();}   // retour au menu principal
    else {http_post_select_object(); strate_menu=3; draw_menu_action();}                               // retour au menu action
    
  }
  else if (strate_menu==3) {                                // ACTION MENU 
    if      (menu_action_value == 0) {strate_menu=2; draw_menu_align_targets();}   
    else if (menu_action_value == 1) {http_post_slew_scope_selected_object();draw_menu_action();}
    else if (menu_action_value == 2) {http_post_slew_scope_center_screen();draw_menu_action();}
    else if (menu_action_value == 3) {http_post_center_screen_selected_object();draw_menu_action();}
    else if (menu_action_value == 4) {strate_menu = 4; draw_menu_scope();}
    else if (menu_action_value == 5) {tft.fillScreen(TFT_BLACK); strate_menu=1; draw_menu_principal();}   // back
    } 
  else if (strate_menu==4) {                                // MENU SELECT SCOPE
    if      (menu_scope_value == 0) {ref_telescope = "1"; tft.fillScreen(TFT_BLACK);}   
    else if (menu_scope_value == 1) {ref_telescope = "2"; tft.fillScreen(TFT_BLACK);}
    else if (menu_scope_value == 2) {ref_telescope = "3"; tft.fillScreen(TFT_BLACK);}
    else if (menu_scope_value == 3) {ref_telescope = "4"; tft.fillScreen(TFT_BLACK);}
    else if (menu_scope_value == 4) {ref_telescope = "5"; tft.fillScreen(TFT_BLACK);}
    strate_menu=3; draw_menu_action();
    } 
 }
 
// gestion quel écran afficher
  if (mode_follow == true) {http_post_slew_scope_center_screen(); strate_menu=555;} 
  if (strate_menu ==0){affiche_selected();}  
  delay(200); 
}


// #######################################################################################################################################
//                                                                          REQUETE GET
// #######################################################################################################################################
void http_get() {
      HTTPClient http;
      String serverPath = serverName + "api/main/status"  ;   
      http.useHTTP10(true);
      http.begin(serverPath.c_str());
      http.GET();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getStream());
      http.end();

      pays = doc["location"]["country"].as<String>();                // pays
      site = doc["location"]["name"].as<String>();                   // site
      longitude = doc["location"]["longitude"].as<String>();         // longitude
      longitude = longitude.substring(0,5);
      latitude =  doc["location"]["latitude"].as<String>() ;         // latitude
      latitude = latitude.substring(0,5);
      date = doc["time"]["local"].as<String>();                      // date locale
      date = date.substring(0,10);
      time_zone = doc["time"]["timeZone"].as<String>();              // Time zone      
      h_locale = doc["time"]["local"].as<String>();                  // Heure locale
      h_locale = h_locale.substring(11,19);
      str10 = doc["selectioninfo"].as<String>();                     // selectionInfo
      
      // extraction des coordonnées Alt/AZ
      az =""; alt=""; int i=0;
      while (str10.substring(i,i+2)!= "Az") {i+=1;}                  // recherche de la position Az/Haut dans la chaine
      i=i+10;    // décalage à la première valeur de Az
      int j=1;
      while (str10.substring(i+j,i+j+1) != "/") {az = az + str10.substring(i+j,i+j+1) ; j+=1;}
      i=i+j; j=1;
      while (str10.substring(i+j,i+j+1) != "<") {alt = alt + str10.substring(i+j,i+j+1) ; j+=1;}
              az.replace("°", "*");                    // cause font 7bits ont pas °
              alt.replace("°", "*");  ;

        
      // extraction type d'objet
      obj_type = ""; i=0; j=0;
      while (str10.substring(i,i+4)!= "Type") {i+=1;} 
      while (str10.substring(i+6,i+10)!= "</b>") {obj_type += str10.substring(i+6,i+7); i+=1;}
      obj_type = obj_type.substring(3);

      //Exctration du nom d'étoile
      nom_etoile = ""; i=0; sort = false;
      if (obj_type.substring(2,7) == "toile"){
      while ((str10.substring(i+4,i+5)!= " ") and (str10.substring(i+4,i+5)!= "<")) {nom_etoile +=str10.substring(i+4,i+5); i+=1;} 
      }

      //Exctration du nom d'une planète
      nom_planete = ""; i=0; sort = false;
      if (obj_type.substring(0,8) == "planète"){
      while (str10.substring(i+4,i+5)!= "<") {nom_planete +=str10.substring(i+4,i+5); i+=1;} 
      }
      else {nom_planete = " ";}
     
      //extraction nom NGC
      nom_ngc = ""; i=0; sort = false;
      while ((str10.substring(i,i+3)!= "NGC") and (sort == false)) {i+=1;if (i==200) {sort=true;}}
      if (sort==false) {while (str10.substring(i+4,i+5)!= " " ) {nom_ngc += str10.substring(i+4,i+5); i+=1; }nom_ngc = "NGC" + nom_ngc;}
      if (sort==true){nom_ngc=" ";}
     
      //extraction nom messier
      nom_m = ""; i=0; sort = false;
      while ((str10.substring(i,i+2)!= "M ") and (sort == false)) {i+=1;if (i==200) {sort=true;}}
      if (sort==false) {while (str10.substring(i+2,i+3)!= " " ) {nom_m += str10.substring(i+2,i+3); i+=1; }nom_m = "M" + nom_m;}
      if (sort==true){nom_m=" ";}
        
      //extraction nom IC
      nom_IC = ""; i=0; sort = false;
      while ((str10.substring(i,i+3)!= "IC ") and (sort == false)) {i+=1;if (i==200) {sort=true;}}
      if (sort==false) {while (str10.substring(i+3,i+4)!= " " ) {nom_IC += str10.substring(i+3,i+4); i+=1; }nom_IC = "IC" + nom_IC;}
      if (sort==true){nom_IC=" ";}
    
      //circum
      circum = false;
      if (str10.indexOf("Circum")>0){circum = true;}

      // lever coucher culmination
      h_lever = ""; h_coucher = ""; h_culmi = ""; i=0;
      i = str10.indexOf("Culmination");
      while (str10.substring(i+11,i+12)!= "<") {h_culmi +=str10.substring(i+11,i+12); i+=1;}       

      //magnitude
      magnitude = ""; i=0;
      i = str10.indexOf("Magnitude");
      while (str10.substring(i+14,i+18)!= "</b>") {magnitude +=str10.substring(i+14,i+15); i+=1;}  


       if (nom_m != " "){obj_selected = nom_m; }
       else if (nom_ngc != " "){obj_selected = nom_ngc; }
       else if (nom_IC != " "){obj_selected = nom_IC; }
       else if (nom_planete != " "){obj_selected = nom_planete; }
       else if (nom_etoile != " "){obj_selected = nom_etoile; }
}

void http_get_j2000(){
      HTTPClient http;
      String serverPath = serverName + "api/main/view?coord=j2000"  ;   
      http.useHTTP10(true);
      http.begin(serverPath.c_str());
      http.GET();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getStream());
      http.end();
      // pour éviter des mouvement intempestif, on prend les coordonnées J2000 et si entre 2 passes z n'a pas changé, on n'envoit pas de nouvel ordre de mouvement
      zj2000= ""; follow = false;
      zj2000 = doc["j2000"].as<String>();
      zj2000 = zj2000.substring(zj2000.indexOf(",")+2);
      zj2000 = zj2000.substring(zj2000.indexOf(",")+4, zj2000.length()-1 );
      intzj2000 = zj2000.toInt();
     // Serial.print("old-new "); Serial.println(String(intzj2000-old_intzj2000));
    //  Serial.print("follow ");  Serial.println(String(follow));
      if ((intzj2000-old_intzj2000) != 0 ) {follow=true;} 
      old_intzj2000 = intzj2000;
}


// #######################################################################################################################################
//                                                                          REQUETES POST
// #######################################################################################################################################
void http_post_center_screen_selected_object(){   // centre l'objet sélectionné sur l'écran Stellarium
    if (obj_selected != " ")  { 
      HTTPClient http3;
      serverPath = serverName + "api/main/focus";   
      requete =""; httpRequestData = ""; httpResponseCode = 0;
      http3.begin(serverPath);
      http3.addHeader("Content-Type", "application/x-www-form-urlencoded");
      requete = "target=" + obj_selected + "&mode=center" ;
      httpRequestData = requete;     
      httpResponseCode = http3.POST(httpRequestData);   
      Serial.print("HTTP Response code: ");Serial.println(httpResponseCode); delay(250);
      http3.end();
      delay(50);
      }
      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB18);tft.drawString("Centering Screen", 160, 120, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(3000);
}

void http_post_slew_scope_center_screen(){  // envoi le télescope vers le centre de l'écran Stellarium
  if ((mode_follow == true) and (follow==false))
  {}
  else {
      HTTPClient http2;
      requete =""; httpRequestData = ""; httpResponseCode = 0;
      serverPath = serverName + "api/stelaction/do";         
      http2.begin(serverPath);
      http2.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpRequestData = "id=actionSlew_Telescope_To_Direction_" + ref_telescope ;           
      httpResponseCode = http2.POST(httpRequestData);  
      Serial.print("HTTP Response code: "); Serial.println(httpResponseCode); delay(250);
      http2.end();  
      delay(50);
  }
  
      if (mode_follow == false){
      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB24);tft.drawString("Centering Scope", 160, 120, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(2000);
      }
      if (mode_follow == true){
      if (follow==true) {tft.setTextColor(TFT_GREEN, TFT_BLACK);tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB12);tft.drawString("*** MOVING ***", 160, 180, GFXFF);}
      if (follow==false) {tft.setTextColor(TFT_ORANGE, TFT_BLACK);tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB12);tft.drawString("*** WAITING ***", 160, 180, GFXFF);}
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(250);
      }
}

void http_post_slew_scope_selected_object(){      // envoi le télescope vers l'objet sélectionné
 if (obj_selected != "") {
      HTTPClient http4;
      requete =""; httpRequestData = ""; httpResponseCode = 0;
      serverPath = serverName + "api/stelaction/do"; 
      http4.begin(serverPath);
      http4.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpRequestData = "id=actionMove_Telescope_To_Selection_" + ref_telescope;           
      httpResponseCode = http4.POST(httpRequestData);  
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      delay(50);    
      }
      
      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB18);tft.drawString("Scope To Selection", 160, 120, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(2000);
}

void http_post_select_object(){                   // sélection un objet céleste
    if (obj_selected != " ")  { 
      HTTPClient http3;
      serverPath = serverName + "api/main/focus";   
      requete =""; httpRequestData = ""; httpResponseCode = 0;
      http3.begin(serverPath);
      http3.addHeader("Content-Type", "application/x-www-form-urlencoded");
      requete = "target=" + menu_align_targets[menu_targets_value] + "&mode=center" ;
      httpRequestData = requete;     
      httpResponseCode = http3.POST(httpRequestData);    
      Serial.print("HTTP Response code: ");Serial.println(httpResponseCode); delay(250);
      http3.end();
      delay(50);
      }

      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB18);tft.drawString("SELECT", 160, 60, GFXFF);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FS18);tft.drawString(menu_align_targets[menu_targets_value], 160, 180, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(2000);  
}

void http_post_align_with_sync(){                  // synchronise le télescope avec l'objet sélectionné dans Stellarium
  if (obj_selected != "") {
      HTTPClient http2;
      requete =""; httpRequestData = ""; httpResponseCode = 0;
      serverPath = serverName + "api/stelaction/do";         
      http2.begin(serverPath);
      http2.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpRequestData = "id=actionSync_Telescope_To_Selection_" + ref_telescope;           
      httpResponseCode = http2.POST(httpRequestData);  
      Serial.print("HTTP Response code: "); Serial.println(httpResponseCode); delay(250);
      http2.end();  
      delay(50);
  }
      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB24);tft.drawString("Align with", 160, 60, GFXFF);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FS18);tft.drawString( nom_etoile, 160, 180, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      delay(3000);  
}

// #######################################################################################################################################
//                                                                          GESTION AFFICHAGE
// #######################################################################################################################################

void affiche_follow(){
      tft.fillRect(10, 10, 300, 220, TFT_WHITE);tft.fillRect(12, 12, 296, 216, TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextDatum(MC_DATUM); tft.setFreeFont(FSB24);tft.drawString("Follow Mode", 160, 60, GFXFF);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void affiche_selected(){
  compteur ++; xpos =  0; ypos = 0;
  if (compteur < 10){info_supp = "Culmin. " + h_culmi;}
  if ((compteur >= 10) and (compteur < 20) ){if (circum==true){info_supp = "Circumpolaire";} else  {info_supp = "Non Circump.";}}
  if ((compteur >= 20) and (compteur < 30) ){info_supp = obj_type.substring(0,25);}
  if ((compteur >= 30) and (compteur < 40) ){info_supp = "Mag: " + magnitude;}
  if ((compteur >= 40) and (compteur < 50) ){info_supp = "Az: " + az;}
  if ((compteur >= 50) and (compteur < 60) ){info_supp = "Alt: " + alt;}
  if (compteur>60) {compteur = 0;}

   tft.fillRect(0, 0, 320, 30, TFT_WHITE);tft.setTextColor(TFT_BLACK, TFT_WHITE);
   tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString("Objet Actuel ", 5, 5, FONT2);
   tft.setTextDatum(TR_DATUM); tft.setFreeFont(FS12);tft.drawString(h_locale, 315, 5, FONT2);tft.setTextColor(TFT_WHITE, TFT_BLACK);
   tft.fillRect(0, 58, 320, 86, TFT_BLACK);tft.setTextDatum(TC_DATUM); tft.setFreeFont(FSB24);tft.drawString(obj_selected, 160, 60, GFXFF);
   tft.setTextDatum(TL_DATUM); tft.setFreeFont(FM9);tft.drawString("Alt:" + alt.substring(0, (alt.indexOf("'")+3)), 5, 130, GFXFF);
   tft.setTextDatum(TR_DATUM); tft.setFreeFont(FM9);tft.drawString("Az:" +  az.substring(0, (az.indexOf("'")+3)), 315, 130, GFXFF);

   tft.drawLine(0, 185, 320, 185, TFT_WHITE);
   tft.fillRect(0, 190, 320, 240, TFT_BLACK);
   tft.setTextDatum(TC_DATUM); xpos= 160; ypos = 200; tft.setFreeFont(FS12);tft.drawString(info_supp, xpos, ypos, GFXFF);    
  }

void affiche_infos_objet(){
  if (circum==true){info_supp = "Circumpolaire";} else  {info_supp = "Non Circump.";}
  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("SELECTION", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FS12);tft.drawString("Name:", 20, 50, GFXFF); tft.setFreeFont(FS12);tft.drawString(nom_IC + nom_etoile + nom_planete + nom_m + "  " + nom_ngc, 120, 50, GFXFF); 
   tft.setFreeFont(FS12);tft.drawString("Type:", 20, 82, GFXFF);   tft.setFreeFont(FS12);tft.drawString(obj_type, 120, 82, GFXFF); 
  tft.setFreeFont(FS12);tft.drawString("Mag:", 20, 114, GFXFF);    tft.setFreeFont(FS12);tft.drawString(magnitude, 120, 114, GFXFF);
 tft.setFreeFont(FS12);tft.drawString("Circum:", 20, 146, GFXFF);    tft.setFreeFont(FS12);tft.drawString(info_supp, 120, 146, GFXFF);
  tft.setFreeFont(FS12);tft.drawString("Culmi:", 20, 178, GFXFF); tft.setFreeFont(FS12);tft.drawString(h_culmi, 120, 178, GFXFF);
  delay(10000);  
}

void affiche_infos_generales(){

  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("GENERAL", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(FS12);tft.drawString("Country:", 20, 50, GFXFF); tft.setFreeFont(FS12);tft.drawString(pays, 120, 50, GFXFF); 
   tft.setFreeFont(FS12);tft.drawString("Site:", 20, 82, GFXFF);   tft.setFreeFont(FS12);tft.drawString(site, 120, 82, GFXFF); 
  tft.setFreeFont(FS12);tft.drawString("Longi:", 20, 114, GFXFF);    tft.setFreeFont(FS12);tft.drawString(longitude, 120, 114, GFXFF);
 tft.setFreeFont(FS12);tft.drawString("Lati:", 20, 146, GFXFF);    tft.setFreeFont(FS12);tft.drawString(latitude, 120, 146, GFXFF);
  tft.setFreeFont(FS12);tft.drawString("Date:", 20, 178, GFXFF); tft.setFreeFont(FS12);tft.drawString(date, 120, 178, GFXFF);
  tft.setFreeFont(FS12);tft.drawString("Zone:", 20, 210, GFXFF); tft.setFreeFont(FS12);tft.drawString(time_zone, 120, 210, GFXFF);
  delay(10000);    
 }


// #######################################################################################################################################
//                                                                          GESTION DES MENUS
// #######################################################################################################################################
void draw_menu_principal(){
  int  m= 0;
  if (menu_value > 4) {m=menu_value-4;} 
  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("MAIN MENU", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString(menu_principal[m], 50, 50, GFXFF);  
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_principal[1+m], 50, 82, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_principal[2+m], 50, 114, GFXFF);    
  tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString(menu_principal[3+m], 50, 146, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_principal[4+m], 50, 178, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(">", 30, ((menu_value-m) * 32 )+50, GFXFF);  
}

void draw_menu_align_targets(){
  int m= 0;
  if (menu_targets_value > 3) {m=menu_targets_value-4;} // permet de scroller
  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("ALIGN MENU", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_align_targets[m], 50, 50, GFXFF);  
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_align_targets[1+m], 50, 82, GFXFF);    
  tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString(menu_align_targets[2+m], 50, 114, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_align_targets[3+m], 50, 146, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_align_targets[4+m], 50, 178, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(">", 30, ((menu_targets_value-m) * 32 )+50, GFXFF);  
}

void draw_menu_action(){
  int m= 0;
  if (menu_action_value > 3) {m=menu_action_value-4;} // permet de scroller
  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("ACTION MENU", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_action[m], 50, 50, GFXFF);  
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_action[1+m], 50, 82, GFXFF);    
  tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString(menu_action[2+m], 50, 114, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_action[3+m], 50, 146, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(menu_action[4+m], 50, 178, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(">", 30, ((menu_action_value-m) * 32 )+50, GFXFF);  
}

void draw_menu_scope(){
  int m= 0;
  if (menu_scope_value > 3) {m=menu_scope_value-4;} // permet de scroller
  tft.fillRect(0, 0, 320, 40, TFT_WHITE);tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_BLACK, TFT_WHITE); 
  tft.setFreeFont(FS12);tft.drawString("SELECT SCOPE", 160, 20, GFXFF); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.fillRect(0, 40, 320, 200, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString("Telescope 1", 50, 50, GFXFF);  
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString("Telescope 2", 50, 82, GFXFF);    
  tft.setTextDatum(TL_DATUM); tft.setFreeFont(FS12);tft.drawString("Telescope 3", 50, 114, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString("Telescope 4", 50, 146, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString("Telescope 5", 50, 178, GFXFF);    
  tft.setTextDatum(TL_DATUM);  tft.setFreeFont(FS12);tft.drawString(">", 30, ((menu_scope_value-m) * 32 )+50, GFXFF);  
}

void initialisation(){
  tft.begin(); tft.setRotation(3);
  
  WiFi.disconnect();                                // si une instance précédente existe, on la ferme
  Serial.begin(115200);
  tft.fillScreen(TFT_BLACK);tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.setTextDatum(TC_DATUM); // Centre text on x,y position
     
      //ECRAN UN
       tft.setFreeFont(FSB24);tft.drawString("Stell' Screen", 160, 30, GFXFF);
       tft.setFreeFont(FSB12);tft.drawString("by", 160, 84, GFXFF);
       tft.setFreeFont(FSB18);tft.drawString("Astr' Au Dobson",160, 126, GFXFF);
       tft.setFreeFont(FSB9);tft.drawString("Version 1.0", 160, 210, GFXFF);  // Draw the text string in the selected GFX free font
      delay(2000);

      // ECRAN DEUX
      tft.fillRect(0, 80, 320, 240, TFT_BLACK);
      tft.setTextDatum(TL_DATUM);
      tft.setFreeFont(FSB12);tft.drawString("Connexion WIFI en cours...", 0, 84, GFXFF);
      
  WiFi.begin(ssid, password); Serial.println("Connecting : Vérifier le WIFI!!!");   // démarrage de la connexion WIFI
  while(WiFi.status() != WL_CONNECTED) {delay(500);}
      tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.setFreeFont(FSB12);tft.drawString("WIFI OK",0, 108, GFXFF);  delay(1000);

      tft.setTextColor(TFT_WHITE, TFT_BLACK);tft.setFreeFont(FSB12);tft.drawString("Connexion Stellarium...",0, 142, GFXFF);
  Serial.println("Connection établie : " + String(ssid));
  obj_selected = "M31";  http_post_center_screen_selected_object(); delay(500);
      tft.setTextDatum(TL_DATUM);
      if (httpResponseCode == 200){tft.setTextColor(TFT_GREEN, TFT_BLACK);tft.setFreeFont(FSB12);tft.drawString("Stellarium OK", 20, 166, GFXFF); delay(1000); } 
      else {tft.setTextColor(TFT_RED, TFT_BLACK);tft.setFreeFont(FSB12);tft.drawString("Erreur  Conn. Stellarium", 20, 166, GFXFF); delay(1000); }   


     tft.setTextColor(TFT_WHITE, TFT_BLACK);
     tft.fillScreen(TFT_BLACK);  
}
