@ECHO OFF

@REM Start of log file related operations
@REM Replace all the invalid special characters in file name with hyphen (-) 
SET TM=%TIME%
SET TM=%TM: =-%
SET TM=%TM::=-%
SET TM=%TM:.=-%
SET TM=%TM:\=-%
SET TM=%TM:/=-%

SET DT=%DATE%
SET DT=%DT: =-%
SET DT=%DT:/=-%
SET DT=%DT:\=-%
SET DT=%DT:.=-%

@REM Check for package contents to ensure the script is not run from a different directory/from zip file without extracting
IF NOT EXIST version.txt  (
    ECHO [ERROR] Unable to find the package contents. Check if you have extracted all the contents of the zip file into a directory and run this script from there.
    ECHO Press any key to exit
    PAUSE > NUL
    EXIT
)

@REM Create directory called logs if not present 
IF NOT EXIST logs (
    MKDIR logs   
)
SET log_filename=logs/log-%DT%-%TM%.txt
ECHO Logs are saved into %log_filename%
@REM End of log file related operations

@REM Get version number
FOR /F "tokens=* delims=" %%x in (version.txt) DO (
    SET VER=%%x
)
ECHO Version: %VER% >> %log_filename%

@REM Check if WINUSB and USBSER drivers are present
SET WINUSB_PRESENT=0
SET USBSER_PRESENT=0
DRIVERQUERY /NH > DriverList.txt
FOR /F "delims= " %%M in ('FINDSTR /I "WINUSB" ^< "DriverList.txt"') DO (
    ECHO WINUSB is present >> %log_filename%
    SET WINUSB_PRESENT=1
)

FOR /F "delims= " %%M in ('FINDSTR /I "USBSER" ^< "DriverList.txt"') DO ( 
    ECHO USBSER is present >> %log_filename%
    SET USBSER_PRESENT=1
)
DEL DriverList.txt

IF %WINUSB_PRESENT%==0 (
    ECHO WINUSB driver is not present. Install Cypress Programmer to install necessary drivers required for programming using KitProg3 - https://www.cypress.com/products/psoc-programming-solutions
)

IF EXIST hex\ (
    setlocal enabledelayedexpansion
    @REM Erase device flash
     ECHO [INFO] Erasing Device Flash
        .\bin\openocd.exe -s .\dependencies\ -f interface\kitprog3.cfg -f target\psoc6_2m.cfg -c "init; reset init; flash erase_sector 2 0 last; flash erase_sector 1 0 last; flash erase_sector 0 0 last; shutdown" >> !log_filename! || (
            ECHO [ERROR] Unable to erase device flash. Close all software accessing the kit, if open.
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
    ECHO [INFO] Device flash erase: Success

    @REM Change directory to hex
    CD hex
    set "hex_file="
    
    @REM Check to flash only cm0p image or flash both CM0p and CM4 images
    IF "%1%"=="cm0p" (
        ECHO [INFO] Programming only CM0p image
        for /r %%F in (cm0p_*.hex) do (
            call set hex_file="%%~NXF"
        )

        @REM Change directory to root
        CD ..

        ECHO [INFO] Programming file !hex_file!
        ECHO [INFO] This might take up to a minute, please wait...

        .\bin\openocd.exe -s .\hex\ -s .\dependencies\ -f interface\kitprog3.cfg -f target\psoc6_2m.cfg -c "program !hex_file! verify; reset run; shutdown" >> !log_filename! || (
            ECHO [ERROR] Unable to Flash Firmware to device
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
        ECHO [INFO] CM0p firmware flash: Success
    ) ELSE IF "%1%"=="" (
        @REM flash both CM0p and CM4 images
        ECHO [INFO] Programming CM0p and CM4 images

        @REM Programming of CM0p image
        IF EXIST cm0p_*.hex (
            for /r %%F in (cm0p_*.hex) do (
                call set hex_file="%%~NXF"
            )
        ) ELSE (
            ECHO [ERROR] CM0p hex file not found in hex directory
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
        @REM Change directory to root
        CD ..
       
        ECHO [INFO] Programming file !hex_file!
        ECHO [INFO] This might take up to a minute, please wait...

        .\bin\openocd.exe -s .\hex\ -s .\dependencies\ -f interface\kitprog3.cfg -f target\psoc6_2m.cfg -c "program !hex_file! verify; shutdown" >> !log_filename! || (
            ECHO [ERROR] Unable to Flash Firmware to device
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
        ECHO [INFO] CM0p firmware flash: Success
        
        @REM Programming of CM4 image
        @REM Change dir to hex
        CD hex
        IF EXIST cm4_*.hex (
                for /r %%F in (cm4_*.hex) do (
                call set hex_file="%%~NXF"
            )
        ) ELSE (
            ECHO [ERROR] CM4 hex file not found in hex directory
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
        @REM Change dir to root
        CD ..

        ECHO [INFO] Programming file !hex_file!

        .\bin\openocd.exe -s .\hex\ -s .\dependencies\ -f interface\kitprog3.cfg -f target\psoc6_2m.cfg -c "program !hex_file! verify; reset run; shutdown" >> !log_filename! || (
            ECHO [ERROR] Unable to Flash Firmware to device
            ECHO Press any key to exit
            PAUSE > NUL
            EXIT
        )
        ECHO [INFO] CM4 firmware flash: Success
    ) ELSE (
        ECHO [ERROR] Invalid command line argument
        ECHO [INFO] Supported Arguments
        ECHO                cm0p        Programs CM0p image only
        ECHO                            [No argument] programs CM0p and CM4 images
    )
) ELSE (
    ECHO [ERROR] Hex directory missing. Create a directory named 'hex' and keep the target hex file inside it.
)

ECHO [INFO] Kit Programming Complete.

PAUSE
