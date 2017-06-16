# BPI-M2-Magic Android 6.0 Source Code 
-------
###1 Build Android BSP

 $ cd lichee
 
 $ ./build.sh config    

Welcome to mkscript setup progress
All available chips:
   1.sun8iw5p1

Choice: 1

All available platforms:
   1. android
   2. dragonboard
   3. linux
   4. camdroid
   
Choice: 1

All available kernel:
   1. linux-3.4
   
Choice: 1

All available boards:
   1. bpi-m2m
   
Choice: 1

   $ ./build.sh 

***********

###2 Buidl Android 

   $cd ../android

   $source build/envsetup.sh
   
   $lunch    //(bpi_m2m-eng or bpi_m2m-user)
   
   $extract-bsp
   
   $make -j8
   
   $pack
