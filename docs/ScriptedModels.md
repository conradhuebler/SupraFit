# Scripted Models

## SupraFit since 15.01.202
Scripted Models are implemented using ChaiScript and can not be used for data with more than one output column ( called series in SupraFit - like several chemical shifts observed in NMR spectra or wavelength etc).

Scripted Models have successfully been tested with Michaelis-Menten kinetics and a polynomial functions.

To test the new functions, get the content of the follow directory:
https://github.com/conradhuebler/SupraFit/tree/master/data/samples

### Test Michaels-Menten
 - Open SupraFit 
 - Load from the above directory MM/MM.dat
 - Add a **New Model** to the workspace as follows:
    - Input data = 1
    - Variables = 2
 - Write into the model definition field the equation: **(A1*X1)/(A2+X1)** and push **Fit**
 
Another way is to open MM.dat and load using **Load Model file** either MM.json or MM-Variables.json

In case of convergency problems: SupraFit uses for the built-in model the Lineweaver-Burk equation as initial guess. This is not (yet) possible. Changing the parameters or performing a **Scan** should help.

### Polynomial fit
As above, but open polynomial/polynomial.dat and use 5 variables (if not all variables are used, SupraFit will not complain). Define your model as
    - A1+X1*A2+A2*pow(X1,2)+A3*pow(X1,3)
    - A1+X1*A2+A2*X1*X1+A3*X1*X1*X1
   
A predefined model is stored in model.json.
etc.

### Performance
ChaiScript and the current implementation (not-row-wise) works well with several threads. The scripted MM model is 5-10 times slower than the built-in model. It effects Monte Carlo simulations with 20k steps.

Have fun and good luck.
## Changing syntax in progress
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

