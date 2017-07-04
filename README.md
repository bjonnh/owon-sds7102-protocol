# owon-sds7102-protocol
This is a collection of tools to manage Owon oscilloscopes under
GNU/Linux (wonder if udev can be used with other OS).

Tested with:
Owon SDS7103
Owon SDS5032 (Thanks Tony R.)

If you need any assistance or have suggestions contact bjonnh-owon at bjonnh.net

## Compile
$ cmake .
$ make

## Run a dump
$ owon-dump -h

## Parse a bin file
$ owon-parse <binfile.bin>

It will output a csv file with the first line describing the data

## Compile with debug mode activated (useful if you have a different model and need help)
$ cmake -DCMAKE_BUILD_TYPE=Debug .
$ make
