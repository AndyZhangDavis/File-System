#!/bin/sh
# make fresh virtual disk
./fs_make.x disk.fs 4096

echo "\nTesting add..."
touch newtest
touch newtest2
touch newtest3
touch newtest123

./test_fs.x add disk.fs newtest
./test_fs.x add disk.fs newtest2
./test_fs.x add disk.fs newtest3
./test_fs.x add disk.fs newtest123

# get fs_info from reference lib
./fs_ref.x ls disk.fs >ref.stdout 2>ref.stderr

# get fs_info from my lib
./test_fs.x ls disk.fs >lib.stdout 2>lib.stderr

# put output files into variables
REF_STDOUT=$(cat ref.stdout)
REF_STDERR=$(cat ref.stderr)

LIB_STDOUT=$(cat lib.stdout)
LIB_STDERR=$(cat lib.stderr)

# compare stdout
if [ "$REF_STDOUT" != "$LIB_STDOUT" ]; then
  echo "Stdout outputs don't match..."
  diff -u ref.stdout lib.stdout
else
  echo "Stdout outputs match!"
fi

# compare stderr
if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
  echo "Stderr outputs don't match..."
  diff -u ref.stderr lib.stderr
else
  echo "Stderr outputs match!"
fi

echo "\nTesting remove..."

./test_fs.x rm disk.fs newtest
./test_fs.x rm disk.fs newtest2

# get fs_info from reference lib
./fs_ref.x ls disk.fs >ref.stdout 2>ref.stderr

# get fs_info from my lib
./test_fs.x ls disk.fs >lib.stdout 2>lib.stderr

# put output files into variables
REF_STDOUT=$(cat ref.stdout)
REF_STDERR=$(cat ref.stderr)

LIB_STDOUT=$(cat lib.stdout)
LIB_STDERR=$(cat lib.stderr)

# compare stdout
if [ "$REF_STDOUT" != "$LIB_STDOUT" ]; then
  echo "Stdout outputs don't match..."
  diff -u ref.stdout lib.stdout
else
  echo "Stdout outputs match!"
fi

# compare stderr
if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
  echo "Stderr outputs don't match..."
  diff -u ref.stderr lib.stderr
else
  echo "Stderr outputs match!"
fi

echo "\nTesting stat on newtest3..."
./test_fs.x stat disk.fs newtest3
./test_fs.x stat disk.fs newtest3

# get fs_info from reference lib
./fs_ref.x stat disk.fs newtest3 >ref.stdout 2>ref.stderr

# get fs_info from my lib
./test_fs.x stat disk.fs newtest3 >lib.stdout 2>lib.stderr

# put output files into variables
REF_STDOUT=$(cat ref.stdout)
REF_STDERR=$(cat ref.stderr)

LIB_STDOUT=$(cat lib.stdout)
LIB_STDERR=$(cat lib.stderr)

# compare stdout
if [ "$REF_STDOUT" != "$LIB_STDOUT" ]; then
  echo "Stdout outputs don't match..."
  diff -u ref.stdout lib.stdout
else
  echo "Stdout outputs match!"
fi

# compare stderr
if [ "$REF_STDERR" != "$LIB_STDERR" ]; then
  echo "Stderr outputs don't match..."
  diff -u ref.stderr lib.stderr
else
  echo "Stderr outputs match!"
fi

# clean
rm disk.fs
rm ref.stdout ref.stderr
rm lib.stdout lib.stderr
rm -rf newtest newtest123 newtest2 newtest3

