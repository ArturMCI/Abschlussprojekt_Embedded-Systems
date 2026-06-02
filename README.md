# Abschlussprojekt_Embedded-Systems

Schiffeversenken-Spiel

## Projekt-Aufbau

```
main.c
│
├── uart.c / uart.h
├── protocol.c / protocol.h
├── crc.c / crc.h
├── field.c / field.h
├── game.c / game.h
└── strategy.c / strategy.h
```

## Projekt-Plan

**UART → Frame → CRC → Nachrichtentypen → Spiellogik → Spielende**

### Durchführung der Tests

* Bis Schritt 6 testest du hauptsächlich mit HTerm.
* Ab Schritt 7 kannst du langsam mit `schiff.py` testen.
* Ab Schritt 9/10 beginnt das eigentliche Spiel.

---

## Minimalanforderungen (60 %)

### 1. UART läuft

**Ziel:** Einzelne Bytes senden und empfangen.

**Test:**
HTerm sendet `A`, STM32 antwortet mit `A`.

---

### 2. Frame senden

**Ziel:** STM32 kann eine Nachricht im Format

```
# | MSG-ID | LEN | PAYLOAD | CRC | $
```

erzeugen.

**Test:**
STM32 sendet z. B. einen STR-Frame.

---

### 3. Frame empfangen

**Ziel:** STM32 erkennt vollständige Frames von `#` bis `$`.

**Test:**
HTerm sendet einen vorbereiteten Frame.

---

### 4. CRC prüfen

**Ziel:** Empfangene Frames werden nur akzeptiert, wenn die CRC stimmt.

**Test:**

* Frame mit richtiger CRC wird angenommen.
* Frame mit falscher CRC wird verworfen.

---

### 5. STR erkennen

**Ziel:** STM32 erkennt eine STR-Nachricht vom Host.

**Test:**
HTerm oder `schiff.py` sendet STR.

---

### 6. STR + Gerätename senden

**Ziel:** STM32 antwortet auf STR mit eigenem Namen.

**Beispiel:**

```
TEAM01
```

---

### 7. CSH empfangen und prüfen

**Ziel:** Host-Prüfsumme empfangen und kontrollieren:

* genau 10 Zeichen
* insgesamt 30 Schiffsteile

---

### 8. Eigene CSH senden

**Ziel:** STM32 berechnet aus dem eigenen hardcodierten Spielfeld die Zeilen-Prüfsumme und sendet sie.

---

### 9. BOO empfangen

**Ziel:** STM32 empfängt Schusskoordinaten vom Host.

**Payload:**

```
Zeile, Spalte
```

---

### 10. Hit/Miss berechnen

**Ziel:** Im eigenen Spielfeld prüfen, ob dort ein Schiffsteil liegt.

**Ergebnis:**

```
H = Hit
M = Miss
```

---

### 11. BMR senden

**Ziel:** STM32 antwortet auf den Host-Schuss mit `H` oder `M`.

---

### 12. Eigenen BOO senden

**Ziel:** STM32 schießt selbst.

**Minimalstrategie:**

```
Feld für Feld durchgehen
```

---

### 13. BMR empfangen

**Ziel:** Antwort des Hosts auf den eigenen Schuss auswerten und speichern.

---

### 14. Spielende + SFR

**Ziel:** Wenn alle Schiffsteile eines Spielers getroffen sind, werden alle 10 Spielfeldzeilen mit SFR übertragen.

---

## Erweiterungen

* Erweiterte Schussstrategie
* Zufällige Schiffsplatzierung
* Spielfeld im RAM verwalten
* Trefferhistorie speichern
* Verbesserte Fehlerbehandlung
* Timeout-Mechanismen
* Wiederholtes Senden bei Kommunikationsfehlern
* Debug-Ausgaben über UART
