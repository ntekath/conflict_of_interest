# Geodaten, Quellen und Lizenzen

**MAP02 konkret importiert:** [Quellen, Lizenzen und Genauigkeitsgrenzen](MAP02.md) sowie `GeoData/Raw/MAP02/sources.lock.json`. Für die tatsächlich verwendeten Europa-Datensätze gilt dieses geprüfte Register einschließlich CC-BY-SA-Wassergeometrie und CC-BY-Hubpunkten. Das Datenpaket ist nicht pauschal Public Domain. MAP01 bleibt der historische Entwurfs-/Hubplan.

**Neue Zielgeografie ab MAP01:** Deutschland/Niederlande 1913. Das aktuelle
[MAP01-Quellenregister](MAP01_Sources.md) und der [Datenplan](MAP01.md) haben für
neue Geografie Vorrang. Die folgenden Natural-Earth-Angaben dokumentieren den
vorhandenen US-Testbestand; moderne Grenzen/Küsten sind keine 1913-Rekonstruktion.

Made with Natural Earth. Kostenlose Kartendaten, keine API-Schlüssel und kein Kartendienst.
Natural Earth stellt seine Kartendaten als Public Domain bereit, einschließlich Bearbeitung und
kommerzieller Verwendung. Freiwillige Attribution ist im Repo und im gekochten Geokatalog enthalten.

- [Nutzungsbedingungen](https://www.naturalearthdata.com/about/terms-of-use/)
- [Länder 1:10m, Version 5.1.1](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-countries/)
- [Orte 1:10m, Version 5.1.2](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-populated-places/)
- [Seen 1:10m, Version 5.0.0](https://www.naturalearthdata.com/downloads/10m-physical-vectors/10m-lakes/)

Die Quellarchive unter GeoData/Raw wurden am 16.11.2026 abgerufen. Version ist Datenveröffentlichung,
nicht Erhebungsdatum jedes Punkts. Original-README und VERSION-Dateien sind innerhalb der ZIPs enthalten.
Direkte NACIS-Downloadadressen und vollständige SHA256-Prüfsummen stehen im maschinenlesbaren
GeoData/Generated/validation.json. Historische Grenzen/Seenstände von 1913 werden damit nicht zugesichert.
„Historic lakes“ von Natural Earth ist keine pauschale 1913-Rekonstruktion und wird hier nicht als solche
verwendet. Es werden nur die fünf benannten natürlichen Großen Seen ausgewählt, keine modernen Stauseen.

Projektionsreferenz: [PROJ Lambert Conformal Conic](https://proj.org/en/stable/operations/projections/lcc.html).
Verwendet wird die dort unterstützte sphärische 2SP-Form (+R), mit dokumentierten eigenen Parametern.
Geodätische Distanzreferenz: [pyproj.Geod](https://pyproj4.github.io/pyproj/stable/api/geod.html).

Importwerkzeuge (keine Runtime-Abhängigkeiten): Shapely 2.1.2 (BSD-3-Clause, GEOS LGPL-2.1),
pyshp 3.0.3 (MIT), pyproj 3.7.2 (MIT, PROJ MIT), mapbox-earcut 2.0.0 (ISC), NumPy 2.4.2
(BSD-3-Clause; zugehörige Drittbibliothekslizenzen in der Distribution). Die exakten Lizenztexte
werden mit den pip-Distributionen installiert; deren Quellprojekte bleiben die maßgeblichen Texte.
Unreal Engine, Engine-BasicShapes und Toolchain sind vorhandene Installationen mit ihren eigenen
Lizenzbedingungen. Es wurden keine zusätzlichen kostenpflichtigen Assets oder Dienste beschafft.
