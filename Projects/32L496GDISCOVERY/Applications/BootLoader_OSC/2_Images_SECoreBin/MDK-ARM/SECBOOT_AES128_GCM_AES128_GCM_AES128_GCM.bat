::Post build for SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM
:: arg1 is the build directory
:: arg2 is the elf file path+name
:: arg3 is the bin file path+name
:: arg4 is the version
:: arg5 when present forces "bigelf" generation
@echo off
set "projectdir=%1"
set "execname=%~n3"
set "elf=%2"
set "bin=%3"
set "version=%4"

set "SBSFUBootLoader=%~d0%~p0\\..\\.."
::The default installation path of the Cube Programmer tool is: "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin"
::If you installed it in another location, please update the %programmertool% variable below accordingly.
set "programmertool="C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\\STM32_Programmer_CLI""
set "userAppBinary=%projectdir%\\PostBuild\\"

set "sfu=%userAppBinary%\\%execname%.sfu"
set "sfb=%userAppBinary%\\%execname%.sfb"
set "sign=%userAppBinary%\\%execname%.sign"
set "headerbin=%userAppBinary%\\%execname%sfuh.bin"
set "bigbinary=%userAppBinary%\\SBSFU_%execname%.bin"

set "nonce=%SBSFUBootLoader%\\2_Images_SECoreBin\\Binary\\nonce.bin"
set "oemkey=%SBSFUBootLoader%\\2_Images_SECoreBin\\Binary\\OEM_KEY_COMPANY1_key_AES_GCM.bin"
set "ecckey=%SBSFUBootLoader%\\2_Images_SECoreBin\\Binary\\ECCKEY.txt"
set "sbsfuelf=%SBSFUBootLoader%\\2_Images_SBSFU\\MDK-ARM\\STM32L496G-Discovery_2_Images_SBSFU\\SBSFU.axf"

::comment this line to force python
::python is used if windows executeable not found
pushd %projectdir%\..\..\..\..\..\..\Middlewares\ST\STM32_Secure_Engine\Utilities\KeysAndImages
set basedir=%cd%
popd
goto exe:
goto py:
:exe
::line for window executeable
echo Postbuild with windows executable
set "prepareimage=%basedir%\\win\\prepareimage\\prepareimage.exe"
set "python="
if exist %prepareimage% (
goto postbuild
)
:py
::line for python
echo Postbuild with python script
set "prepareimage=%basedir%\\prepareimage.py"
set "python=python "
:postbuild

::Make sure we have a Binary sub-folder in UserApp folder
if not exist "%userAppBinary%" (
mkdir "%userAppBinary%"
)

set "command=%python%%prepareimage% enc -k %oemkey% -n %nonce% %bin% %sfu% > %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

set "command=%python%%prepareimage% sign -k %oemkey% -n %nonce% %bin% %sign% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error
set "command=%python%%prepareimage% pack -k  %oemkey%  -r 72 -v %version% -n %nonce% -f %sfu% -t %sign% %sfb% -o 512 >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

set "command=%python%%prepareimage% header -k  %oemkey%  -r 72 -v %version% -n %nonce% -f %sfu% -t %sign% -o 512 %headerbin%  >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

set "command=%python%%prepareimage% merge -i %headerbin% -s %sbsfuelf% %elf% %bigbinary% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

IF  "%~5"=="" goto :finish
echo "Generating the global elf file (SBSFU and userApp)"
echo "Generating the global elf file (SBSFU and userApp)" >> %projectdir%\output.txt
set "command=%programmertool% -ms %elf% %headerbin% %sbsfuelf% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

:finish
::backup and clean up the intermediate file
del %sign%
del %sfu%
del %headerbin%

exit 0

:error
echo "%command% : failed" >> %projectdir%\\output.txt
:: remove the elf to force the regeneration
if exist %elf%(
  del %elf%
)
echo %command% : failed

pause
exit 1

:nothingtodo
exit 0
