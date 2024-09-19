# https://nickdesaulniers.github.io/blog/2018/06/02/speeding-up-linux-kernel-builds-with-ccache/
make CC="ccache gcc" -j$(nproc)
sudo make modules_install
sudo make install
sudo update-grub
git diff b8cea2e630cf0b935ebb76aa56f26a98f5eee33b...main -- . ':!build.sh'  ':!ptrace-examples' > lab1.patch
cp lab1.patch ../submission-lab1-cse582/
