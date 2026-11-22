# AP09.5a – Prüfnachweise

Stand 22.11.2026, Paketbaseline 5c138ab.

- `TargetedTests.txt`: vier gezielte AP09.5a-Tests bestanden.
- `FullTests.txt`: 108/108 Gesamttests einschließlich derselben vier AP09.5a-Tests und bestehender Wirtschafts-, Banken-, Handels-, Karten- und UI-Regression.
- `TaxSaveContinuation.txt`: erweiterter 35-Tage-Test nach kompletter Steuerfestsetzung/-zahlung, Save/Load und weiteren 48 identischen Stunden. Keine doppelte Monatsabgabe.
- `EditorBuild.txt`, `GameBuild.txt`, `ShippingBuild.txt`: letzter regulärer Build jeweils Result: Succeeded. Standardmodulmanifest wieder auf reguläre DLL-Namen gesetzt.
- `35days.csv`: tägliche Bilanz-/Beschäftigungs-/Zahlungsmesswerte. `BudgetDay35.txt`: beide nationalen Abschlussstände des 35. Tages, vor den zusätzlichen zwei Save-Fortsetzungstagen.
- `Reproducibility.json`: byteidentische Offline-Szenariogenerierung, alle privaten Definitionen/Startdaten/Regeln gegenüber MAP05 unverändert. Ausschließlich zwei erklärte Staatskonten mit zusammen 650.000 Cent Reserve sowie entsprechender Bank-Eröffnungsgegenposten; keine laufenden Geld-/Warenzugaben.

## Native Sichtprüfung

Vorhandene Unreal-Spielvorschau, Skill computer-use, 1280×720 Clientgröße mit Fensterrahmen. Regionale Beschäftigung und nationales Stellenangebot, Einklappen/Gruppendetails und Scrollen geprüft. In Amsterdam: private/Banklöhne 7,50 USD/Tag, öffentliche Löhne 3,60 USD/Tag für zwei Stellen, Transfer 0,00 USD/Tag; die niedrige Gruppe hat dort tatsächlich keine Lohnzahlung. Der Screenshot `720_income.png` zeigt die echten veröffentlichten Werte der hohen Gruppe. Der zu lange Bezeichner in der Gesamtzeile wurde anschließend ohne Datenänderung auf „Einkommen gesamt“ gekürzt und neu gebaut. Keine Prüfung bei 1080p oder eines gestagten Installers in diesem Paket.

Die Vorschau war zeitweise parallel bedient. Nach ausdrücklicher Freigabe wurden die abschließenden Klicktests erledigt und das Fenster geschlossen. Zwei versuchte separat benannte Hot-Reload-Module führten beim zusätzlichen Teststart zu doppelten Klassenregistrierungen; sie waren kein erfolgreicher Testlauf. Die endgültige Zusatzprüfung erfolgte erfolgreich nach regulärem Neubau mit geschlossenem Vorschaufenster. Temporäre Build-/Crashdateien bleiben im ignorierten Entwicklungsbereich; sie sind keine Save-Migration oder Änderung der Partie.

Nicht installierte SDKs anderer Plattformen und eine mögliche Timeout-Warnung der Engine-Verbindungsprüfung sind keine fehlgeschlagenen Win64-Prüfungen. Erforderliches Win64-SDK ist gültig. Kein neuer Cook/Installer-Lauf. Die allgemeinen Regeln zu beschädigten/inkompatiblen Saves werden unverändert zusätzlich durch die bestehenden Gesamttests geprüft.
