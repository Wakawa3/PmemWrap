sudo make uninstall prefix=/home/satoshi/testlib -j$(nproc)
make clean -j$(nproc)
sudo make install prefix=/home/satoshi/testlib -j$(nproc)
