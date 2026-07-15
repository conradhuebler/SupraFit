# Session Handoff — Thermogramm-GUI-Routing (2026-07-15)

Branch: **`refactor/thermo-gui-routing`**, abgezweigt von `bugfix/thermo-nmr-chart-fixes` (`c2e0b0c7`).
Alles unten ist **committet**. Der Branch ist **nicht gepusht** (kein Upstream).

> Nicht verwechseln mit `SESSION_HANDOFF.md` — die gehört zur Speciation-Sitzung vom 2026-07-11 und
> enthält den noch offenen D9-Befund (zwei Fenster, dieselben Projekte). Sie ist unangetastet.

Vollständiger Plan mit den Details zu C8–C11: `/home/conrad/.claude/plans/iridescent-waddling-forest.md`

---

## Ausgangslage

Der halbfertige „move-to-core"-Umbau hatte den `ItcProcessor` (`src/core/itcprocessor.{h,cpp}`)
angelegt und ihm die beiden `ThermogramHandler` gegeben — aber der Dialog **rief keine einzige
Processor-Methode auf**. Der Processor war verkabelt und komplett inert: `m_use_dilution` immer
`false`, sein Volumen-Vektor immer leer, `resultChanged()` ohne Zuhörer. `Thermogram::UpdateTable()`
war eine Parallel-Implementierung von `recomputeNetHeat()`.

Ziel: Der Core besitzt die Wissenschaft, die GUI rendert sie.

---

## Erledigt (8 Commits, neueste zuerst)

| Commit | Inhalt |
|---|---|
| `b92c01ef` | **C7** — `UpdateTable()` ist reiner Renderer; Dilutions-Gating gesetzt; `Content()`/Export über `ResultRows()`; tote Member raus |
| `d094c52f` | **C6** — nur das Experiment definiert die Titration (Systemparameter-Stomp, Bug 2) |
| `bdb29334` | **C5** — Processor besitzt die Injektionsvolumina (Bug 1); `ImportRow`-Crash (Bug 3) |
| `29c54ec0` | **C4** — `ToolSet::LoadITCFile` Out-Param-Vertrag + `data/samples/itc/synthetic.itc` |
| `89145939` | **C3** — `ItcProcessor`-API für die GUI + Tests |
| `1eb29c68` | **C2** — ITC-Guard bei leerer Regression + libpeakpick-Pointer |
| `4a40cf0b` | **C1** — libpeakpick auf Upstream + schnellere Integration |
| `6adeb3cf` | **C0** — Pin-Test für die absoluten ITC-Integrale |

**Tests:** `test_itc_thermogram` 15/15 grün (vorher 7). GUI + CLI bauen.

**Bugs erledigt:** 1 (Volumina-Append), 2 (Systemparameter-Stomp), 3 (`ImportRow`-Crash),
5 (ungeguardeter `item(i,0)`-Zugriff). **Offen: Bug 4** (Dilutions-Provenance) → C10.

### libpeakpick

Submodul steht auf `be6623b`, **auf origin/master gepusht** (`ecba3cb..d8e32b9..be6623b`).
Zwei Commits von mir plus vier Upstream-Commits, auf denen SupraFit vorher nicht stand.

---

## ⚠️ Deine Aufgaben

### 1. GUI manuell gegenprüfen (blockiert das Weitermachen nicht, aber den Merge)

Der Rendering-Pfad hat **keinen Harness** — der Pin-Test deckt nur den Core ab. Priorisiert:

| # | Prüfung | Erwartung |
|---|---|---|
| 1 | **`.itc`-Experiment laden** | Spalte 0 zeigt die Volumina der Datei, wie auf `master` |
| 2 | **Experiment laden, dann Dilution laden** | Spalte 0 **und** die vier Konzentrationsfelder bleiben unverändert (das war Bug 1 + 2) |
| 3 | **Spalte 3 gegen `master` vergleichen** | identisch, mit und ohne Dilution |
| 4 | **Dilutions-Feld leeren** | Spalte 3 springt auf die Experiment-Werte zurück — **auf `master` passiert das nicht**. Verhaltensänderung: gewollt? |
| 5 | **Kommentar-nur-Datei über „Import Row"** | Warnung statt Crash |
| 6 | **`.dh`- und `.dat`-Export** | vorher/nachher identische Bytes |
| 7 | **Dilutions-Serie im Chart** | erscheint nur mit geladener Dilution; „Show Dilution" funktioniert |

### 2. Entscheidungen, die ich nicht treffen kann

- **F5 (in C10 fällig):** Heute schreibt `Raw():711` den Dilutions-`file` **unbedingt**, vor
  `File2JsonBlock`. Mit `StoreFileName` **aus** behält die Dilution damit einen Pfad, während das
  Experiment keinen speichert. Der Fix macht beide symmetrisch und **entfernt** ihn dann.
  → Empfehlung: Symmetrie nehmen (die Asymmetrie ist versehentlich; ohne Experiment-Pfad lädt das
  Projekt ohnehin nicht). **Braucht dein OK.**
- **C11:** Das Aufräumen löscht auch auskommentierte Blöcke (`thermogram.cpp:99-100,153-161,165-183,
  228-241`) — geparkte Features (Frequenz-Override, Per-Datei-Offset-Anzeige). Git bewahrt sie.
  **Braucht dein OK.**

### 3. Dein Arbeitsbaum (uncommitted, unangetastet gelassen)

```
 M CMakeLists.txt          <- ich habe NUR `-static` entfernt (auf deine Freigabe)
 M misc/SupraFit.desktop
 M src/ui/widgets/preparewidget.cpp
```

`-static` brach den GUI-Link gegen dein dynamisches Qt6
(`attempted static link of dynamic object libQt6Qml.so`). `-static-libgcc -static-libstdc++` sind
geblieben. **Falls du portable Binaries brauchst:** `-static` ist dafür der falsche Hebel, solange Qt
shared gelinkt wird — das ist noch offen.

### 4. Nebenbefund, nicht angefasst

`test_pipeline` **linkt nicht** (`DataFactory::`-Symbole fehlen im Target). Vorbestehend, unabhängig
von dieser Arbeit, gegen den Basis-Commit verifiziert.

---

## Als Nächstes: C8 → C9 → C10 → C11

### C8 — Uniform-Volumen-Checkbox *(entschieden: explizite Checkbox)*

Der heutige `forceInject`-Toggle (`thermogram.cpp:187-196`) kippt das Flag bei **jedem Tastendruck** —
der effektive Wert hängt an der Zeichenzahl („12.5" tippen → true, false, true, false). Ein treuer
Port würde den Bug mitportieren.

- Neue `QCheckBox* m_uniformInject` („Uniform volume for all injections"), `m_injct` nur aktiv wenn
  gecheckt; Paritäts-Toggle und `m_forceInject` löschen.
- Neu `ResolveInjectionVolumes()`, zuerst aus `UpdateData()`: gecheckt → `setUniformInjectionVolume()`,
  sonst → `padInjectionVolumes()`.
- Per-Zell-Edit löst uniform (erhält `:263`), `setExperimentFile` resettet (erhält `:364`).
- `m_message` ehrlich machen, inkl. der bisher **stillen** Dilutions-Größen-Divergenz
  („Dilution deckt nur %1 von %2 Injektionen ab").

> **C8 löst zugleich eine offene Kopplung:** `Content()` rendert derzeit bewusst aus `ResultRows()`,
> **nicht** aus `resultTable()`. Grund: `resultTable()` liest den Volumen-Vektor direkt und liefert
> `0.0` für Zeilen, die er nicht abdeckt, während die Tabelle auf das Inject-Feld zurückfällt — das
> hätte **Tabelle und Modell auseinanderlaufen lassen**. Sobald `ResolveInjectionVolumes()` den Vektor
> vollständig auflöst, stimmen beide überein und der Umstieg auf `resultTable()` wird ehrlich. Der
> Kommentar in `Content()` sagt das.

> **Bewusster Trade-off in C8:** eager Broadcast, kein Render-Zeit-Override. Checkbox an→aus stellt die
> Datei-Volumina **nicht** wieder her (Datei neu laden). Das ist der Preis des aufgelösten Vektors —
> und der ist es, der die Volumina überhaupt erst persistierbar macht (C10).

### C9 — Gekoppelte Skalierung *(entschieden: gekoppelt)*

`ThermogramWidget`: Combo → `ScalingFactorChanged(qreal)`-Signal, `setScalingFactor(qreal)`-Slot als
reine View (QSignalBlocker), Ctor-Seed raus. **`LoadDefault()` (`:611-617`) muss adoptieren statt zu
stampfen** — es schob bisher den Combo-Default in den Handler, weshalb ein projekt-gespeicherter
`ScalingFactor` **jeden Reload nicht überlebte** (F2). Dialog koppelt beide über
`m_processor->setScalingFactor()` (ist idempotent → keine Rückkopplung).

### C10 — JSON *(entschieden: `toJson()` + Legacy-Fallback)*

- `Raw()` = `m_processor->toJson()` + vom Dialog gemergte Provenance. **Der Core bekommt keinen
  Hook**: die Regeln lesen `qApp`-Properties, und `ItcProcessor` läuft im CLI, wo die ungesetzt sind.
  Core besitzt den wissenschaftlichen Inhalt, Dialog dekoriert mit Storage-Policy.
- `File2JsonBlock` (`:680-698`) sein `filename`-Argument benutzen lassen statt hartkodiert
  `m_exp_file->text()` → **Bug 4**.
- `setRaw()`: `fromJson()`, dann **Dilution-Fit VOR Dilution-File** (F1 — `Initialise()` läuft
  *innerhalb* `setDilutionFile`, sah die gespeicherten Parameter also nie), Vektor danach wieder
  anlegen.
- `fromJson()` liest den Legacy-Skalar bereits (in C3 gebaut + getestet).
- **Härteste Anforderung:** alte `.suprafit`-ITC-Projekte müssen unverändert laden.

### C11 — Aufräumen + Doku

Tote Member (`m_exp_therm`, `m_dil_therm`, `m_injection`, `m_forceStep`, `m_heat_offset`,
`m_dil_offset`, `m_offset`, `m_refit`, `m_scale`, `m_exp_base`, `m_dil_base`, `m_freq`), die nie
definierten `PickPeaks()`/`fromSpectrum()`, `thermogramhandler.h:38-39` `Series()`/`Baseline()` +
`m_integrals_list`. Copyright → `2016 - 2026`. `AIChangelog.md`, `src/ui/CLAUDE.md`,
`src/core/CLAUDE.md`. Instructions-Eintrag *„move all thermogramm analysis to core / or finalise the
move"* streichen, falls du ihn als erledigt siehst.

### C12 *(zurückgestellt, eigener Review)* — `ImportData`: DataTable direkt übergeben

`importdata.cpp:563-595` und `:597-629` sind Fast-Duplikate. Der `Content()`-String-Hop schickt jede
Wärme durch `QString::number(double)` → **6 signifikante Stellen** und parst zurück. Direkte Übergabe
behält volle Präzision — **ändert damit Zahlen** (letzte Stellen), deshalb separat.

---

## Track B — libpeakpick-Mathe (eigener Branch, nach Track A)

Zahl-**ändernd**, deshalb getrennt. Der Pin-Test aus C0 trägt hinüber und macht jedes Delta sichtbar.
Prinzip: **eine Zahl-Änderung pro Commit**, Delta im Commit-Text begründet.

- **B0 — Eigen entkoppeln** *(Blocker für alles Strukturelle)*: `CMakeLists.txt:210` legt
  `external/libpeakpick/eigen/` global als `SYSTEM`-Include an — es ist SupraFits **einziges** Eigen
  (kein `find_package(Eigen3)`). Verschiebt/entfernt man libpeakpick, verliert SupraFit Eigen.
- **B1 — `Peak.end`-Konvention** *(der Kern)*: SupraFit baut inklusiv
  (`setPeakEnd(i + timestep - 1)`, `thermogramhandler.cpp:267`), `FindMaximum`/`FindMinimum`
  (`analyse.h:96,110`) lesen exklusiv, `IntegrateNumerical` läuft `i < end - 1`; `baseline.h:272,404`
  liest inklusiv. → Konvention festlegen, Leser angleichen, **Pin-Test-Delta messen → dein Urteil**.
  *Magnitude ehrlich:* das fallengelassene Trapez liegt am Peak-Ende, wo ein abgeklungenes ITC-Signal
  wieder auf der Baseline sitzt — für getrennte Injektionen erwarte ich wenig. Der Off-by-one in der
  Max/Min-Suche ist davon unabhängig real.
- **B2 — UB → definiert**: `spectrum.h:170-176`/`:198-204` (`i >= size() && i < 0` ist **nie** wahr →
  `X(-1)` liest OOB; live aus `thermogramhandler.cpp:416`), `XtoIndex` (`while (i < 0) x++;` — `i`
  ändert sich nie), `baseline.h:432` uninitialisierter `m_peaks`, `baseline.h:454/468/475`
  `peak->end - 1` auf `unsigned` → `UINT_MAX`.
- **B3 — zahl-ändernd, je ein Commit**: `baseline.h:484` (`|| gradient - 1` ist **immer wahr** → das
  Gradienten-Kriterium ist tot, live in `thermogramhandler.cpp:557`); `mathhelper.h:124,155`
  (`meanThreshold`/`stddevThreshold` teilen durch die **volle** Größe); `baseline.h:267,286`
  (`Corrected()` schiebt um ein Sample); `baseline.h:358-362` (`m_baseline` im `no_coeff==2`-Zweig nie
  zugewiesen → subtrahiert nichts); `baseline.h:109-118` (`diff` nie genullt → immer 100 LM-Schritte).
- **B4 — Hygiene**: `spectrum` Rule-of-Zero (User-Dtor + Copy-Ops unterdrücken **Moves**; jede Kopie
  re-`Analyse()`t O(n)), `getRangedSpectrum` schreibt OOB, Stale-Cache nach `center()`/`InvertSgn()`.
- **Zurückgestellt mit FIXME im Code**: `nxlinregress.h:56` `initial[i]` statt `initial[j]`. Für
  `functions=3` folgenlos (der Zweig setzt `Value()[0]=1`, was die Schleife sofort abbricht), aber ab
  **4** macht der Fix ganze Konfigurations-Familien gültig, die heute verworfen werden — und
  `regressionanalysisdialog.cpp` lässt den Nutzer bis `Datenpunkte/2` Funktionen wählen.

---

## Was eine neue Session wissen sollte

- **Der Pin-Test ist der Wächter.** `testAbsoluteIntegralsPinned` nagelt die 20 unskalierten Integrale
  auf `reaction.dat` fest. Bewegt er sich in Track A, hat das Routing die Wissenschaft verändert →
  **Stopp und Ursache klären**. Er darf sich nur in Track B bewegen, begründet. Toleranz 1e-9, weil die
  OpenMP-Reduktion pro Lauf im letzten Bit streut (~1e-16). Nicht die Zahlen anpassen, damit er grün wird.
- **`git stash` fasst Submodul-Inhalte nicht an.** Ein A/B-Test über `git stash` an libpeakpick
  vergleicht nichts — die Änderung bleibt drin. Das hat mich in dieser Sitzung einmal fast in die Irre
  geführt.
- **Kaltstart vs. warm.** Der erste Testlauf nach einem Build ist ~3x langsamer. Für Perf-Aussagen:
  Minimum aus ~10 Läufen, isolierter Test.
- **`setDilutionEnabled()` integriert nicht.** Es wählt, ob eine **bereits integrierte** Dilution am
  Join teilnimmt. Auf einer nie integrierten subtrahiert es lautlos Null. Am Setter dokumentiert.
- **Änderungsklassen** (das Sortierprinzip dieses Branches): (1) echt zahl-neutral → Track A;
  (2) UB-Beseitigung, Ergebnis gleich wo heute definiert → Track A; (3) zahl-ändernd → **Track B**,
  mit gemessenem Delta.
