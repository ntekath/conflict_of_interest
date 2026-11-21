# MAP05 – Prüfnachweise

Stand 21.11.2026, Paketbaseline 4ca320c.

- `FullTests.txt`: 104/104 Gesamttests, darunter 4 neue MAP05-Tests.
- `FinalTargetedTests.txt`: 4/4 MAP05-Tests nach der letzten Text-/Symbolanpassung. Die anschließende Absicherung für fehlgeschlagene Kartenprojektionen wurde nochmals in allen drei Zielkonfigurationen gebaut.
- `EditorBuild.txt`, `GameBuild.txt`, `ShippingBuild.txt`: letzter Build jeweils Result: Succeeded.
- `Delivery.txt`: tatsächliche 100-kg-Testlieferung, 98 kg nach Verderb, 1.119 Cent Fremdfracht, 932 Cent Betrieb, zwei konkrete Zubringer, einmalige Ankunft.
- `Reproducibility.json`: Szenariogenerierung byteidentisch; Eröffnungskonten, Waren und Bevölkerung unverändert. Genau ein deklariertes neues Schiffsasset mit 2.000.000 Cent Buchwert, zwei vorhandene Lkw in den Hafendienst umgewidmet. MAP04-Netz unverändert.

## Native Sichtprüfung

Skill `computer-use`, vorhandene Unreal-Spielvorschau. 1280×720 als Fenster mit Rahmen, 1920×1080 im Vollbild. Keine Bildbearbeitung; originale UI-Screenshots.

- `720_quote.png`: komplette scrollbare Wasserpreis-/Umschlagvorschau, erreichbare Bestätigung, noch keine Buchung. Aufnahme vor der letzten rein redaktionellen Textkorrektur.
- `720_active.png`: gebuchter Auftrag, tatsächliche Spielzeit Stunde 8, Wasserkette unterwegs. Die Abschnitts-ID wurde anschließend im Produkt durch verständliche Orts-/Abschnittsnamen ersetzt.
- `720_route.png`: Netzlinie und exemplarisches Schiff im Kartenausschnitt. Kein neu erzeugtes Simulationsfahrzeug durch die Anzeige. Der finale Marker wurde zusätzlich auf den Wasserteil begrenzt.
- `1080_quote.png`: finale 1080p-Vorschau mit Kapazitäten, Kostenaufteilung, Warte-/Umschlagzeit und erreichbaren Buttons.

Bei 720p wurden Auswahl, Vorschau, ausdrückliche Bestätigung, deaktivierte erneute Bestätigung, Start/Pause, aktive Kette, gebundene Hafen-Lkw und Karte/Zoom geprüft. Bei 1080p wurden die finale Auswahlliste, Vorschau, Scrollen und Schaltflächen geprüft, ohne einen weiteren Auftrag abzusenden. Testfenster danach geschlossen. Ankunft, Storno, beschädigte Metadaten und Save/Load während der Fahrt wurden automatisiert geprüft.

Die SDK-Inventarisierung meldet nicht installierte SDKs anderer Plattformen; Win64 ist gültig. Die Engine-Verbindungsprüfung kann eine harmlose HTTP-Timeout-Warnung protokollieren. Netz/Transport brauchen keine Internetverbindung. Kein neuer Cook/Installer-Test in MAP05.
