# StellScreen : Firmware Arduino (ESP32) pour Stellarium
Ce projet est un firmware codé en Arduino utilisant une carte ESP32 + un écran TFT 2" ST7789 permettant de communiquer avec le logiciel Stellarium : obtenir des données, envoyer des commandes.

![](https://github.com/AstrAuDobson/StellScreen-Firmware-Arduino-ESP32-pour-Stellarium/blob/main/ecran_principal.jpg)

Actuellement, il vous sera possible de : <br>
-Obtenir des données comme le site d'observation (nom, longitude, latitude), la date, l'heure, la time zone, l'objet céleste sélectionné et diverses caractéristiques (magnitude, type, position AltAz).

-Envoyer des commandes de contrôle : centrer l'écran sur un objet, sélectionner un objet, bouger le télescope connecté, synchroniser le télescope sur un objet...
La possibilité de maintenir le télescope sur la position centrale de l'écran a permis de créer la fonction "FOLLOW" qui vous permettra de naviguer avec votre souris dans le ciel, le télescope suivra le mouvement.
