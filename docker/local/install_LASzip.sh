#!/bin/bash
git clone -n https://github.com/LASzip/LASzip.git && cd LASzip 
git checkout 585a940c8d80f039fd1294ecd1411440938d7241 && ./autogen.sh && ./configure && make all -j 4 && sudo make install  &&
sudo mkdir /usr/local/include/laszip/ && sudo mv /usr/local/include/las*.hpp /usr/local/include/laszip/
