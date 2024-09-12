# https://nickdesaulniers.github.io/blog/2018/06/02/speeding-up-linux-kernel-builds-with-ccache/
make CC="ccache gcc" -j$(nproc)
sudo make modules_install
sudo make install
sudo update-grub
