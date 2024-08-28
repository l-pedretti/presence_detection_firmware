arm-none-eabi-objcopy -O binary ./build/CYSBSYSKIT-DEV-01/Debug/*.elf ./build/CYSBSYSKIT-DEV-01/Debug/CP.bin
chmod 777 ./tools/sign_image/keytools/sign
# [[ ${host} = win ]]     && ./tools/sign_image/keytools/sign.exe --custom --ed25519 --sha256 ./build/CYSBSYSKIT-DEV-01/Debug/CP.bin ./tools/sign_image/keys/ed25519.der 1
./tools/sign_image/keytools/sign --custom --ed25519 --sha256 ./build/CYSBSYSKIT-DEV-01/Debug/CP.bin ./tools/sign_image/keys/ed25519.der 1

dd if=/dev/zero bs=524283 count=1 2>/dev/null | tr "\000" "\377" > ./build/CYSBSYSKIT-DEV-01/Debug/cp_fota_image.bin
dd if=./build/CYSBSYSKIT-DEV-01/Debug/CP_v1_signed.bin of=./build/CYSBSYSKIT-DEV-01/Debug/cp_fota_image.bin bs=1 conv=notrunc
printf "pBOOT" >> ./build/CYSBSYSKIT-DEV-01/Debug/cp_fota_image.bin
rm -rf ./build/CYSBSYSKIT-DEV-01/Debug/CP.bin