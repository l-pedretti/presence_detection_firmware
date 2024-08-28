# Kit Programmer for Windows

 Programming tools for development kit. This tool is meant for Windows only.

## Instructions 
- Run the ```program_kit.cmd``` file
### If you edit the code example
- Copy the compiled hex file from the build directory of your code example project to the ```hex``` directory.
- If programming only the CM0p image
    - Open a terminal in this working directory containing the *program_kit.cmd* file
    - Execute the command ``` program_kit.cmd cm0p ```
- Else 
    - Run the ```program_kit.cmd``` file which programs the cm0p image and cm4 image
### Expected Output
```
Logs are saved into logs/log-28-10-2021-11-07-49-63.txt
[INFO] Erasing Device Flash
[INFO] Device flash erase: Success
[INFO] Programming CM0p and CM4 images
[INFO] Programming file "cm0p_1_1_7.hex"
[INFO] This might take up to a minute, please wait...
[INFO] CM0p firmware flash: Success
[INFO] Programming file "cm4_quick-iot-experience_0_21_0.hex"
[INFO] CM4 firmware flash: Success
[INFO] Kit Programming Complete.
Press any key to continue . . .
```
## Version History

| Version | Description of change |
| ------- | --------------------- |
| 1.0.0   | First version of kit programmer bundle      |
| 1.1.0   | Error message is printed on the console if the script is run from archived (zip) file    |
