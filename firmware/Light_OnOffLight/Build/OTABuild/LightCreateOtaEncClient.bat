#
# $1 : the OTA buildd directory
# $2 : the SDK directory name
# $3 : the manufacturer code
# $4 : 32 byte OTA header string
# $5 : OTA Device Id


# Change the path to the OTA Build folder.
cd $1

# ####################################################################################################:
# ####################################Build Encrpted Client Binary##################################################:

# Add serialisation Data with ImageType = 0x1XXX - Indicates it is for Encrpted devices
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m combine -f Light.bin -x configOTA_6x_Cer_Keys_HA_Light.txt -v JN516x -g 1 -k 0xffffffffffffffffffffffffffffffff -u $3 -t $5  -j $4 

# Creat an Unencrpted Bootable Client with Veriosn 1
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --embed_hdr -c outputffffffffffffffff.bin -o Client.bin -v JN516x -n 1 -u $3 -t $5  -j $4 

# Creat an Encrypted Image from the Bootable UnEncrpted Client - This must be copied to Ext Flash and then erase internal Flash
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m bin -f Client.bin -e Client_Enc.bin -k 0xffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 -v JN516x 

# ###################Build OTA Encrypted Upgarde Image from the Unencrypted Bootable Client Image #########################:
# Modify Embedded Header to reflect version 2 
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV2.bin -v JN516x -n 2 -u $3 -t $5  -j $4 

# Now Encrypt the above Version 2  
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m bin -f UpGradeImagewithOTAHeaderV2.bin -e UpGradeImagewithOTAHeaderV2_Enc.bin -v JN516x -k ffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 

# Wrap the Image with OTA header with version 2
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV2_Enc.bin -o Client_UpgradeImagewithOTAHeaderV2_Enc.bin -v JN516x -n 2 -u $3 -t $5  -j $4 
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV2_Enc.bin -o Client_UpgradeImagewithOTAHeaderV2_Enc.ota -p 1 -v JN516x -n 2 -u $3 -t $5  -j $4 

# Modify Embedded Header to reflect version 3 
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --embed_hdr -c Client.bin -o UpGradeImagewithOTAHeaderV3.bin -v JN516x -n 3 -u $3 -t $5  -j $4 

# Now Encrypt the above Version 3  
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m bin -f UpGradeImagewithOTAHeaderV3.bin -e UpGradeImagewithOTAHeaderV3_Enc.bin -v JN516x -k ffffffffffffffffffffffffffffffff -i 0x00000000000000000000000000000000 

# Wrap the Image with OTA header with version 3
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV3_Enc.bin -o Client_UpgradeImagewithOTAHeaderV3_Enc.bin -v JN516x -n 3 -u $3 -t $5 -j $4
../../../../../SDK/$2/Tools/OTAUtils/JET.exe -m otamerge --ota -c UpGradeImagewithOTAHeaderV3_Enc.bin -o Client_UpgradeImagewithOTAHeaderV3_Enc.ota -p 1 -v JN516x -n 3 -u $3 -t $5 -j $4

# ####################################################################################################:
# #################################### Clean Up Imtermediate files##################################################:

# rm Light.bin 
rm output*.bin
rm UpGradeImagewithOTAHeader*.bin

chmod 777 Client.bin
chmod 777 Client_UpgradeImagewithOTAHeaderV2_Enc.bin
chmod 777 Client_UpgradeImagewithOTAHeaderV3_Enc.bin
