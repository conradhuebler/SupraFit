# Scripted Models

## Changing syntax in progress

Using the latest commit, function definitions reduces to
```json
"ChaiScript": {
      "1": "for(var i = 0; i < series; ++i){for(var j = 0; j < points; ++j) {Output[i][j] = (vmax*S[j]/(Km+S[j]));}}"
    }
```
as fast definiton and
```json
"ChaiScript": {
       "1": "for(var i = 0; i < series; ++i){for(var j = 0; j < points; ++j) {Output[i][j] = (vmax*Input[i][j]/(Km+Input[i][j]));}} "
```
as slow definiton. The actual difference is the handling of the input block. While S is the input vector with the length of points, Input is matrix with one column and points rows.

SupraFit translates the above code internally to
```json
def Calculate()
{
    var Output = Output_const;
    for(var i = 0; i < series; ++i){for(var j = 0; j < points; ++j) {Output[i][j] = (vmax*Input[i][j]/(Km+Input[i][j]));}}
    return Output;
}
```

The results have to be written into the Output matrix.

At the current state, direct manipulation of the script can be done in the model widget.

Using commit **55f90e0** the function to define the model is called for every data point * rows - currently only Michaelis Menten therefore rows = 1.
```json
"ChaiScript": {
    "1" : "def Calculate(int i, int j) { return vmax*S[j]/(Km+S[j]); }"
    }
```

With the most recent commit, the function is called row-wise:
```json
"ChaiScript": {
     "1": "def Calculate(int series) ",
     "2": "{",
     "3": "var vector = [];",
     "4": "vector.resize(S.size());",
     "6" : "for(var j = 0; j < S.size(); ++j) {vector[j] = (vmax*S[j]/(Km+S[j])); }",
     "7": "return vector",
     "8": "}"
    }
```
At the current state, the resulting vector has to be definied and resize!

## General
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
suprafit_cli -j cli_script.json
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

Threading is not supported, at least using the thread-safe mode in ChaiScript makes parallel excution much slower and serial.
By removing thread-safety, parallel excution becomes faster, but leads to stranges crahes.

