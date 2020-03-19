# Importing Thermograms using suprafit_cli

In case of *.itc files, thermograms can automatically processed via simple

```sh
suprafit_cli -i thermogram.itc -o thermogram.suprafit
```

Only *.suprafit or *.json files can be exported (for now), *.dH still have to be exported using the thermogram dialog.

## WIP - Stuff

Documentation will be added later.

```sh
suprafit_cli -j job.json
```

Job File:
```json
{
    "main": {
    "InFile" : "thermogram.dat",
    "OutFile" : "project",
    "IndependentRows": 1,
    "Prepare" :{
        "Integration" : {
        "PeakCount" : 30,
        "InjectVolume" : 4,
        "PeakDuration"  : 1800,
        "CalibrationHeat" : 0.05,
        "CalibrationStart" : 3600
            }
        }
    }
}
```
