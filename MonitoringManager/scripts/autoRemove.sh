#!/bin/bash

LOGFILE="/var/log/autoRemove.log"
MAINDIR="/mnt/intenso/MONITORING"
TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
maxCatalogs=10
maxDetectionFiles=10000

ALTANKA_CAM="altanka_cam"
ALTANKA_DET="altanka_det"
BRAMA_CAM="brama_cam"
BRAMA_DET="brama_det"


echo "$TIMESTAMP start autoremove script" >> $LOGFILE

countDir=$(ls -d $MAINDIR/$ALTANKA_CAM/*/ 2>/dev/null | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

if [ $countFilesToRemove -ge 1 ]; then

    catalogsToRemove=$(ls -d $MAINDIR/$ALTANKA_CAM/*/ | head -${countFilesToRemove})
    for eachCatalog in $catalogsToRemove
    do
        rm -rf $eachCatalog
        echo "$TIMESTAMP ALTANKA: removed $eachCatalog" >> $LOGFILE
    done
fi

countDir=$(ls -d $MAINDIR/$BRAMA_CAM/*/ 2>/dev/null | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

if [ $countFilesToRemove -ge 1 ]; then
    catalogsToRemove=$(ls -d $MAINDIR/$BRAMA_CAM/*/ | head -${countFilesToRemove})
    for eachCatalog in $catalogsToRemove
    do
        rm -rf $eachCatalog
        echo "$TIMESTAMP BRAMA: removed $eachCatalog" >> $LOGFILE
    done
fi


TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
echo "$TIMESTAMP finished autoremove script" >> $LOGFILE

# Clean up detection result files to keep exactly $maxDetectionFiles
# altanka_det
countFiles=$(find $MAINDIR/$ALTANKA_DET -maxdepth 1 -type f 2>/dev/null | wc -l)
if [ $countFiles -gt $maxDetectionFiles ]; then
    filesToRemove=$(($countFiles-$maxDetectionFiles))
    find $MAINDIR/$ALTANKA_DET -maxdepth 1 -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | head -${filesToRemove} | cut -d' ' -f2- | while read file; do
        rm -f "$file"
    done
    echo "$TIMESTAMP ALTANKA_DET: removed $filesToRemove files, now $maxDetectionFiles)" >> $LOGFILE
fi

# brama_det
countFiles=$(find $MAINDIR/$BRAMA_DET -maxdepth 1 -type f 2>/dev/null | wc -l)
if [ $countFiles -gt $maxDetectionFiles ]; then
    filesToRemove=$(($countFiles-$maxDetectionFiles))
    find $MAINDIR/$BRAMA_DET -maxdepth 1 -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | head -${filesToRemove} | cut -d' ' -f2- | while read file; do
        rm -f "$file"
    done
    echo "$TIMESTAMP BRAMA_DET: removed $filesToRemove files, now $maxDetectionFiles)" >> $LOGFILE
fi

# a lot of logs here. clear it daily
#truncate -s 0 /var/log/proftpd/vroot.log
#truncate -s 0 /var/log/auth.log
