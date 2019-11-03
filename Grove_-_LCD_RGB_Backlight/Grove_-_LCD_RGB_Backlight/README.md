# Azure Sphere library for Grove LCD RGB Backlight
This library allows use of [Grove - LCD RGB Backlight](http://wiki.seeedstudio.com/Grove-LCD_RGB_Backlight/) on Azure Sphere devices and kits. This library is a ported version of the Grove Arduino library with the same name.

## Importing
1. Start Visual Studio. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
1. Select the file CMakeLists.txt and then click **Open**
1. From the **CMake** menu (if present), select **Build All**. If the menu is not present, open Solution Explorer, right-click the CMakeLists.txt file, and select **Build**. This will build the application and create an imagepackage file. The output location of the Azure Sphere application appears in the Output window.
1. From the **Select Startup Item** menu, on the tool bar, select **GDB Debugger (HLCore)**.
1. Press F5 to start the application with debugging. The string "Hello, world!" will be displayed on the first line and a counter on the second line.

## Information
The Grove connector on the Azure Sphere MT3620 Starter Kit provides 3.3V VCC, only. The LCD display **requires** 5V, so you have to rewire VCC to a 5V pin if you wish to use the Grove connector on the MT3620.
