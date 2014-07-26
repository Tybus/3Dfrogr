name=${PWD##*/}
make
./resources/makerom -f cci -rsf ./resources/gw_workaround.rsf -target d -exefslogo -elf $name.elf -icon ./resources/icon.bin -banner ./resources/banner.bin -o $name.3ds
./resources/makerom -f cia -o $name.cia -elf $name.elf -rsf ./resources/build-cia.rsf -icon ./resources/icon.bin -banner ./resources/banner.bin -exefslogo -target t 
