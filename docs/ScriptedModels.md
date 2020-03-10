# Scripted Models

Scripted Models can be defined by a simple json file, where the main part is done in a "ModelDefinition" block:
```json
"ModelDefinition": {
    "DepModelNames": "v",
    "Python": {
        "1": "def Calculate(i,j):",
        "2": "     S = input[j][i]",
        "3": "     return vmax*S/(Km+S)"
    },
    "ChaiScript" : {
        "1" : "def Calculate(int i, int j) { return vmax*S[j]/(Km+S[j]); }"
    },
    "GlobalParameterNames": "vmax|Km",
    "GlobalParameterSize": 2,
    "InputNames": "S",
    "InputSize": 1,
    "LocalParameterSize": 0,
    "Name": "Py-Michaelis-Menten-Protoyp"
}
```

Loading a scripted model using suprafil_cli and the files that are located at data/samples/

```sh
suprafil_cli -j cli_script.json
```

```json
{
    "Main": {
    "InFile" : "test.dat",
    "OutFile" : "scriptmodel",
    "Guess" : true,
    "Fit"   : true,
    "InputSize" : 1,
    "Threads" : 12,
    "Extension" : "json"
    },
    "Models" :{
        "1" :{
            "ScriptModel" :{
                "GlobalParameterSize" : 2,
                "GlobalParameterNames" : "vmax|Km",
                "LocalParameterSize" : 0,
                "InputSize"  : 1,
                "Name" : "Scripted-Michaelis-Menten-Protoyp",
                "InputNames" : "S",
                "DepModelNames": "v",
                "Python": {
                    "1": "def Calculate(i,j):",
                    "2": "     S = input[j][i]",
                    "3": "     print (Km)",
                    "4": "     print (vmax)",
                    "6": "     print (vmax*S/(Km+S))",
                    "7": "     return vmax*S/(Km+S)"
                    },
                "ChaiScript" : {
                    "1" : "def Calculate(int i, int j) { return vmax*v[j]/(Km+v[j]); }"
                    }
                }
            }
        },
    "Jobs"  :
    {


    }
}
```

test.dat must be data set havin both independent and dependent variables.
