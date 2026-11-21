# MAP04 – Prüfnachweise

Stand 21.11.2026, Baseline aa2b659.

- `TargetedTests.txt`: 4/4 MAP04-Tests.
- `FullTests.txt`: 100/100 Gesamttests einschließlich derselben vier MAP04-Tests. Testprozess erfolgreich beendet.
- `EditorBuild.txt`, `GameBuild.txt`, `ShippingBuild.txt`: jeweils Result: Succeeded.
- `Route.txt`: tatsächlicher Rückgabewert der C++-Planungsfunktion, Hin-/Rückdauer und vollständige Kantenfolge.
- `Reproducibility.json`: zwei identische Offline-Kompilationen, byteidentische Szenariogenerierung, eingebetteter Graph identisch; Wirtschaftsszenario ohne neue Netzdefinition exakt gleich Paketbaseline. Keine zusätzlichen Wirtschaftsassets, Anfangsgelder oder Waren.

Die SDK-Inventarisierung meldet nicht installierte SDKs anderer Plattformen. Geprüft wird hier Win64 mit gültigem Windows-SDK. Diese Meldungen sind keine fehlgeschlagenen Win64-Builds oder Tests. Der Engine-Testlauf enthält eine harmlose Timeout-Warnung seiner Internet-Verbindungsprüfung; Netzkompilierung und Route benötigen keine Internetverbindung.

Headless Automation, keine neue Desktop-Sichtprüfung oder Paketierung in diesem Paket. Die bestehende Diagnose bekommt Textdaten; neue Wasserbuchungen und Kartenanimationen sind MAP05. Reproduktion und historische Quellen/Genauigkeitsgrenzen siehe [MAP04](../../MAP04.md).
