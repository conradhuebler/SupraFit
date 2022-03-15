# Implementing own Models in SupraFit

## Prior to all

- First thing, SupraFit should be successfully compileable
- Programming skills in C++ and Qt are mandatory
- If everything worked well, and a SupraFit file (*.json, *.suprafit) with the results based on the custom model is shared with other, they need the same source code to reproduce the obtained results. To respect the FAIR principles, the changes should finally be included in the SupraFit code.

In case of question, open an issue at Github, start a discussion or write me an email.

## How-To

The SupraFit model source code files can be found at **src/core/models** and directories therein. For example take **mm_model.cpp** and **mm_model.h**, which are to calculate Michaelis-Menten kinetics. Copy both and give them a new name, like **custom_model.cpp** and **custom_model.h**.

- Within these files, rename the classes to your new name and adopt the name of the **#include "mm_model.moc"** at the end of the cpp file (**#include "custom_model.moc"**)
- Include the header-file in **src/core/models/models.h**
- Assign the model a unqiue number (enum) in **src/global.h** as well as a name. Look in both functions **inline SupraFit::Model Name2Model(const QString& str)** and **inline QString Model2Name(SupraFit::Model model)**.
- Adopt that enum in the function **virtual inline SupraFit::Model SFModel() const** in the models header file
- Make SupraFit aware of that model by adopting the **inline QSharedPointer<AbstractModel> CreateModel(int model, QPointer<DataClass> data)** function in the **src/core/models/models.h** file. The model class has to fit the enum.

Now SupraFit should be aware of the model. Finally, add the **custom_model.cpp** to the CMakeLists.txt and compile SupraFit. Open SupraFit after successful compilation, check the Settings dialog and set **Show advanced options and simulation tools in GUI** to true. Load a data set and push the button **Any Model** and specify the number (enum) assigned previously.
If everything worked well, now the model source code be adjusted to the model function needed. The model calculation is finally be done in the protected virtual function **virtual void CalculateVariables()**.

After that, make a pull request to merge the model with the main source code of SupraFit. As the models are identified by the enum number given in **src/global.h**, this ensures other can work the model and results obtained.

## Alternative way

Open an issue on github with a worked example or literature reference and some data and it will be considered to be included in SupraFit.

## Scripted Models

The development branch contains some work towards ScriptedModels. The model is then stored in the json or suprafit file and can be shared and therefore respect the FAIR principles.
