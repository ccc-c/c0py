set -x
make clean
make

cp ../xv7/kernel kernel.bin
cp ../xv7/fs.img fs.img
# ./scripts/download.sh

make check

