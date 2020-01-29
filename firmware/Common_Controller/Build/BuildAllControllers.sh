#!/bin/bash
#Usage below
#    1. make sure that environment Path variable must contain C:\NXP\bstudio_nxp\msys\bin in the beginning!
#    2. ./BuildAllControllers.sh <JN5168 or JN5169>
echo "Building Controllers from JN-AN-1171"

start=$(date +"%T")
echo "Start Time : $start"

if [ "$1" == "JN5168" ] || [ "$1" == "JN5169" ]; then
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_ColorController         clean >> BuildLog_Controller_ColorController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_ColorSceneController    clean >> BuildLog_Controller_ColorSceneController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_NonColorController      clean >> BuildLog_Controller_NonColorController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_NonColorSceneController clean >> BuildLog_Controller_NonColorSceneController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_OnOffSensor             clean >> BuildLog_Controller_OnOffSensor_$1.txt &
    echo "Cleaning; Please wait"
    wait

    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_ColorController         >> BuildLog_Controller_ColorController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_ColorSceneController    >> BuildLog_Controller_ColorSceneController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_NonColorController      >> BuildLog_Controller_NonColorController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_NonColorSceneController >> BuildLog_Controller_NonColorSceneController_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 REMOTE=Controller_OnOffSensor             >> BuildLog_Controller_OnOffSensor_$1.txt &
    echo "Building; Please wait"
    wait
else
    echo "Usage ./BuildAllControllers.sh <JN5168 or JN5169>"
fi

end=$(date +"%T")
echo "End Time : $end"

echo "Done !!!"
