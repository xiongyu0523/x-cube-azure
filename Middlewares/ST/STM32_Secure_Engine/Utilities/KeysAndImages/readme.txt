=================================
Overview
=================================
The python scripts contained in this directory are used by pre and post processing scripts in the SE_CoreBin and UserApp projects.

1) These scripts will generate the appropriate .s files implementing the functions declared in se_key.h.
2) They are also used by the post-processing script to  generate the FW image and header files (.sfu and .sfuh) and the single binary file (header + FW image).

To use these scripts you need to make sure you have installed the appropriate python modules listed in "requirements.txt":
> pip install -r requirements.txt

Please note that you do not need to call these scripts manually as they are integrated in the IDE.
They will be called as pre and post build scripts when building the projects with your IDE.


=================================
'prepareimage.py' functionalities
=================================
The 'prepareimage.py' script is the main item of this folder.
This script can be used to perform 5 types of operations:

* generate some keys ('keygen' command):
      ** symmetric key for Firmware encryption (AES CBC or AES GCM) and authentication when AES GCM is used.
      ** asymmetric keys for authentication when ECDSA is used.

* generate the ARM assembler code to load the keys from FLASH into RAM
      This is the 'trans' command.
      It generates the .s file to build in the context of the SE_CoreBin project.

* encrypt the firmware image ('enc' command)
      This is a symmetric encryption: AES CBC or AES GCM.

* generate the tag to authenticate the firmware ('sign' command or 'sha256' command)
      ** When AES GCM is used for authentication, this is the "sign" command: generates the FW tag.
      ** When SHA256 with ECDSA is used for authentication, this is the "sha256" command: generates the sha256 digest

* generate the Firmware header (metadata) and the single binary file to be downloaded (header+ clear or encrypted firmware )
      This is the 'pack' command.


=================================
Some examples
=================================

Example for AES-CBC:
--------------------
[1] Generate the keys - OPTION
python prepareimage.py keygen -k AES_CBC.bin -t aes-cbc
=> for aes_cbc , "AES_CBC"  must be in file name else key is not created, this
is used to discriminate AES_GCM key versus AES_CBC key
python prepareimage.py  keygen  -k ECCKEY.txt -t ecdsa-p256

This step is not mandatory but if you do so, please make sure:
* to copy the content of  ECCKEY.txt in SECoreBin\Binary
* to use AES_CBC.bin instead of OEM_KEY_COMPANY1_key.bin
Then the prebuild operations (SE_CoreBin) must be performed.

[2] Encrypt the image
python prepareimage.py  enc -k AES_CBC.bin -i iv.bin UserApp.bin UserApp.sfu

[3] Generate the clear FW tag (SHA256 of the clear FW)
python prepareimage.py sha256  UserApp.bin  UserApp.sign

[4] Generate the .sfb FW metadata (header) and encrypted binary
python prepareimage.py  pack -k ECCKEY.txt -r 4 -p 1 -v 2 -i iv.bin -f UserApp.sfu -t UserApp.sign UserApp.sfuh
(Please note the use of -r 4 to have a FW header length of 128 bytes. This is needed to match the FLASH constraint.)

The step 1 is optional, the steps 2,3 and 4 are handled by the post-build scripts already integrated in the IAR IDE.

Example for AES-GCM :
---------------------
[1] Generate the keys - OPTION
python prepareimage.py keygen -k OEM_KEY_COMPANY1_key.bin  -t aes-gcm

This step is not mandatory but if you do so, please make sure:
* to copy the content OEM_KEY_COMPANY1_key.bin  in SECoreBin\Binary
Then the prebuild operations (SE_CoreBin) must be performed.

[2] Encrypt the image
python prepareimage.py  enc -k OEM_KEY_COMPANY1_key.bin -n nonce.bin UserApp.bin UserApp.sfu

[3] Generate the clear FW tag with AES-GCM
python prepareimage.py sign -k OEM_KEY_COMPANY1_key.bin -n nonce.bin UserApp.bin  UserApp.sign

[4] Generate the .sfb FW metadata (header) and encrypted binary
python prepareimage.py  pack -k OEM_KEY_COMPANY1_key.bin  -r 4 -p 1 -v 2 -n nonce.bin -f UserApp.sfu -t UserApp.sign UserApp.sfuh
(Please note the use of -r 4 to have a FW header length of 128 bytes. This is needed to match the FLASH constraint.)

The step 1 is optional, the steps 2,3 and 4 are handled by the post-build scripts already integrated in the IAR IDE.

=================================
Windows executable(s)
=================================
Two windows executables are provided:
(a) Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/win/prepareimage.exe
    ==> this file contains all the required element to run but is slower at execution time.

(b) Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/win/prepareimage/prepareimage.exe
    ==> this file requires additional items (provided in its sub-folder) but is faster at execution time.

By default, the scripts use (b).

=================================
Rebuilding windows executable
=================================
It is also possible to generate windows executables to avoid using the python scripts directly.

This requires some additional Python libraries and tools:
>pip install pypiwin32
>pip install pywin32-ctypes
>pip install altgraph
>pip install pefile

use python 3
go in directory Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages
>git clone https://github.com/pyinstaller/pyinstaller.git
>cd pyinstaller
>git checkout develop

Follow method (a) or (b).
Method (a) produces a single file while method (b) provides a folder (execution can be faster).

(a) Building in "one file" mode:
>python pyinstaller.py --onefile ../prepareimage.py
>cp -p prepareimage/dist/prepareimage.exe ../win/

OR

(b) Building in "onefolder" mode:
>python pyinstaller.py --clean --onedir ../prepareimage.py
Then copy the folder "Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/pyinstaller/prepareimage/dist/prepareimage"
in "Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/win"

Then decide if you want to use:
(a) Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/win/prepareimage.exe
or
(b) Firmware/Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages/win/prepareimage/prepareimage.exe
and update the scripts accordingly.