# ★★★★★ WANTED LEVEL
### Sunshine State Distortion, di Tune Talks

Un plug-in di distorsione/saturazione (VST3, AU e Standalone) con **un solo knob a 5 stelle**.
Più alzi il livello di ricercato, più la situazione degenera.

| Stelle | Stato | Cosa succede |
|---|---|---|
| 0 | CLEAN RECORD | segnale pulito |
| ★ | PETTY THEFT | saturazione morbida a valvola |
| ★★ | GRAND THEFT | più drive, armoniche asimmetriche, inizia la compressione |
| ★★★ | ARMED & LOUD | hard clip, bitcrusher, compressione pesante, luci della polizia |
| ★★★★ | STATEWIDE MANHUNT | riduzione del sample rate e sirene filtrate in sottofondo che girano in stereo |
| ★★★★★ | MAXIMUM CHAOS | wavefolding, crush a 4 bit, compressione 20:1, sirena "yelp" impazzita, l'interfaccia trema |

Il knob è continuo, quindi puoi automatizzarlo in modo fluido durante build-up e drop.
Nel plug-in trovi anche:
- **le stelle cliccabili**: clicca una stella per saltare a quel livello; riclicca la stessa per scendere di una.
- **MIX**: dry/wet, per la distorsione in parallelo.
- **OUTPUT**: da -24 a +12 dB.

Le sirene partono solo quando c'è audio in ingresso, quindi non suonano nel silenzio.

---

## Installazione

Scarica l'installer dalla pagina **Releases** del repository (oppure da **Actions** -> ultima esecuzione -> **Artifacts**).

| Sistema | File | Cosa installa | DAW |
|---|---|---|---|
| macOS 11+ (Intel e Apple Silicon) | `WantedLevel-x.y.z-macOS.pkg` | VST3, AU, app Standalone (facoltativa) | Logic Pro, GarageBand, Ableton Live, e altre |
| Windows 10/11 64 bit | `WantedLevel-x.y.z-Windows-Setup.exe` | VST3, app Standalone | Ableton Live, FL Studio, Cubase, Reaper, e altre |

**Logic Pro:** il plug-in si trova in *Audio Units -> Tune Talks -> Wanted Level*.
Se non compare, apri il Terminale, esegui `killall -9 AudioComponentRegistrar` e riapri Logic.

**Ableton Live:** in *Preferenze -> Plug-in* attiva "Usa cartella di sistema VST3" (e su Mac anche "Usa Audio Units"), poi premi *Rescan*.

**Avviso di sicurezza su macOS:** finché il pacchetto non è firmato con un certificato Apple Developer, la prima volta macOS può bloccarlo.
In quel caso fai clic destro sul `.pkg` e scegli **Apri**, oppure vai in *Impostazioni di Sistema -> Privacy e sicurezza* e scegli **Apri comunque**.

## Come pubblicare una nuova versione
```bash
git tag v1.0.0
git push origin v1.0.0
```
GitHub compila tutto, testa il plug-in (con `auval` per Logic e `pluginval` per il VST3) e pubblica una Release con i due installer allegati.

## Compilare in locale

### macOS (VST3 + AU)
Servono Xcode (o i Command Line Tools) e CMake (`brew install cmake`).
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
installer/mac/build_pkg.sh build/WantedLevel_artefacts/Release 1.0.0   # crea dist/*.pkg
```

### Windows (VST3)
Servono Visual Studio 2022 (con "Sviluppo di applicazioni desktop C++"), CMake e Inno Setup 6.
```bat
cmake -B build -A x64
cmake --build build --config Release
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\windows\WantedLevel.iss
```

> Alla prima configurazione CMake scarica JUCE 8 da GitHub, quindi serve una connessione a internet.

---

## Struttura
```
Source/WantedDSP.h          motore audio in C++ puro (catena di distorsione, sirena, compressore)
Source/PluginProcessor.*    parametri, salvataggio dello stato, collegamento con la DAW
Source/PluginEditor.*       grafica: tramonto, palme, fenicottero, gabbiani, stelle HUD, luci della polizia
Assets/                     scritte in Pricedown pre-renderizzate (PNG)
tools/                      script che genera le immagini delle scritte
installer/mac/               script e configurazione dell'installer .pkg
installer/windows/           script Inno Setup dell'installer .exe
```

## Font (Pricedown)
Il titolo, gli stati e le etichette usano il font **Pricedown** di Typodermic Fonts, quello dei titoli di GTA.
La licenza gratuita non permette di inserire il font dentro un software, per questo il font **non** è incluso nel plug-in.
Le scritte sono immagini PNG già pronte nella cartella `Assets/`.

Per cambiare le scritte, scarica il font (gratis da dafont.com o typodermicfonts.com) e rigenera le immagini:
```bash
pip install pillow
python3 tools/render_text_assets.py "percorso/Pricedown Bl.otf"
```

## Nota legale
Il plug-in ha un'estetica *ispirata* al crime e alla Florida anni '80/2020, ma non usa nomi, loghi, font o asset di Rockstar Games o di GTA.
Se lo distribuisci, tienilo così. JUCE ha la sua licenza (AGPLv3 oppure commerciale): controlla le condizioni prima di vendere il plug-in.
