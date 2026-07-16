# Session Handoff — Thermogramm-GUI-Routing (2026-07-16)

Branch: **`refactor/thermo-gui-routing`**, abgezweigt von `bugfix/thermo-nmr-chart-fixes` (`c2e0b0c7`).
Alles unten ist **committet**. Der Branch ist **nicht gepusht** (kein Upstream). **Track A ist
vollständig (C0–C11).**

> Nicht verwechseln mit `SESSION_HANDOFF.md` — die gehört zur Speciation-Sitzung vom 2026-07-11 und
> enthält den noch offenen D9-Befund (zwei Fenster, dieselben Projekte). Sie ist unangetastet.

Vollständiger Plan: `/home/conrad/.claude/plans/iridescent-waddling-forest.md`

---

## Ausgangslage

Der halbfertige „move-to-core"-Umbau hatte den `ItcProcessor` (`src/core/itcprocessor.{h,cpp}`)
angelegt und ihm die beiden `ThermogramHandler` gegeben — aber der Dialog **rief keine einzige
Processor-Methode auf**. Der Processor war verkabelt und komplett inert: `m_use_dilution` immer
`false`, sein Volumen-Vektor immer leer, `resultChanged()` ohne Zuhörer. `Thermogram::UpdateTable()`
war eine Parallel-Implementierung von `recomputeNetHeat()`.

Ziel: Der Core besitzt die Wissenschaft, die GUI rendert sie. **Erreicht.**

---

## Erledigt — Track A vollständig (12 Commits, neueste zuerst)

| Commit | Inhalt |
|---|---|
| `d97d482b` | **C11** — tote Member/Decls raus, Copyright + Doku (AIChangelog, CLAUDE.md) |
| `36119511` | **C10** — JSON über `toJson()`/`fromJson()` (Bug 4, F1); F5 als Verhaltensänderung markiert |
| `9de1c44a` | **C9** — ein cal→J-Faktor, von Experiment + Dilution geteilt (behebt F2) |
| `a45a2c96` | **C8** — explizite „Uniform volume"-Checkbox statt Paritäts-Toggle |
| `b92c01ef` | **C7** — `UpdateTable()` ist reiner Renderer; Dilutions-Gating; `Content()`/Export über `ResultRows()` |
| `d094c52f` | **C6** — nur das Experiment definiert die Titration (Systemparameter-Stomp, Bug 2) |
| `bdb29334` | **C5** — Processor besitzt die Injektionsvolumina (Bug 1); `ImportRow`-Crash (Bug 3) |
| `29c54ec0` | **C4** — `ToolSet::LoadITCFile` Out-Param-Vertrag + `data/samples/itc/synthetic.itc` |
| `89145939` | **C3** — `ItcProcessor`-API für die GUI + Tests |
| `1eb29c68` | **C2** — ITC-Guard bei leerer Regression + libpeakpick-Pointer |
| `4a40cf0b` | **C1** — libpeakpick auf Upstream + schnellere Integration |
| `6adeb3cf` | **C0** — Pin-Test für die absoluten ITC-Integrale |

**Tests:** `test_itc_thermogram` 15/15 grün (vorher 7). GUI + CLI bauen.
**Alle fünf Bugs erledigt** (1, 2, 3, 5 im Code; 4 in C10) + F1 + F2.

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

### 2. Entscheidungen — zwei davon habe ich vorläufig getroffen, du kannst sie kippen

- **F5 (in C10, bereits umgesetzt):** `Raw()` schrieb den Dilutions-`file` **unbedingt**. Mit
  `StoreFileName` **aus** behielt die Dilution damit einen Pfad, während das Experiment keinen
  speichert. Jetzt sind beide symmetrisch — der Dilutions-Pfad wird in diesem Modus **nicht mehr**
  geschrieben. Ich habe die empfohlene Symmetrie genommen (die Asymmetrie war versehentlich; ohne
  Experiment-Pfad lädt das Projekt ohnehin nicht) und im Commit `36119511` sichtbar markiert.
  **Falls du das anders willst, hier rückgängig machen.**
- **Geparkte Feature-Blöcke (in C11 bewusst NICHT gelöscht):** die auskommentierten Blöcke in
  `thermogram.cpp` (Frequenz-Override `m_freq`, `m_scale`, Per-Datei-Offset `m_exp_base`/`m_dil_base`,
  `m_refit`) und ihre Deklarationen **stehen noch** — Löschen geparkter Features wollte ich dir
  überlassen. Sag Bescheid, dann raus (git bewahrt sie).

### 2b. GUI-Verifikation gegen `master` — Vorsicht

`master` hat den `ItcProcessor` noch nicht (der entstand auf `bugfix/thermo-nmr-chart-fixes`). Für
den Spalte-3-Vergleich ist die ehrliche Baseline **`bugfix/thermo-nmr-chart-fixes`** (der eingefrorene
Ausgangspunkt), nicht `master`.

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

## Als Nächstes

Track A (C0–C11) ist **fertig**. Was bleibt:

1. **GUI manuell gegenprüfen** (Abschnitt „Deine Aufgaben" oben) — der Rendering-Pfad hat keinen
   Harness. Das ist die Voraussetzung für den Merge.
2. **F5** ggf. kippen und **geparkte Feature-Blöcke** ggf. löschen (beides oben).
3. **C12** (unten) — Präzisions-Fix, eigener Review.
4. **Track B** (unten) — libpeakpick-Mathe, eigener Branch, ändert Zahlen.

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
