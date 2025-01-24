cd pmdk
rm -rf pmdk_install
mkdir pmdk_install
sudo make uninstall prefix=/home/satoshi/artifact/PmemWrap/pmdk_install -j$(nproc)
make clean -j$(nproc)
sudo make install prefix=/home/satoshi/artifact/PmemWrap/pmdk_install -j$(nproc)
