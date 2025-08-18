mkdir -p build
cd build
cmake ..
ninja
timeout 10s ./MyFirmware || ( [[ $? -eq 124 ]] && echo "Done ok!" )
rm MyFirmware
