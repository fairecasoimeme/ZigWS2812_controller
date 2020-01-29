cd ../OTABuild

# ::::::::::::::::::::::::::::::::::::::::::Build Server Binary ::::::::::::::::::::::::::::::::::::::::::::::::::::
# :: REM add serial data to the Coordinator binary  
../../../../../SDK/JN-SW-4168/Tools/OTAUtils/JET.exe -m combine -f OTAServer_JN5168.bin -x configOTA_6x_Cer_Keys_HA_Server.txt -v 4 -g 1 -k 11111111222222223333333344444444 -u 0x1037 -t 0x5168

# :: Creat a server file with default header that is already there as part of rom place holder version 1
../../../../../SDK/JN-SW-4168/Tools/OTAUtils/JET.exe -m otamerge --embed_hdr -c output0000000000000002.bin -o Server.bin -v 4 -n 1 -u 0x1037 -t 0x5168
 
rm -f OTAServer_JN5168.bin
rm -f output0000000000000002.bin