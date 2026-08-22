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

# Clean up detection result files to keep exactly $maxDetectionFiles.
# The files are stored in date-named subdirectories, so we must scan recursively.
clean_detection_dir() {
    local dir="$1"
    local label="$2"

    if [ ! -d "$dir" ]; then
        return
    fi

    countFiles=$(find "$dir" -type f 2>/dev/null | wc -l)
    if [ "$countFiles" -gt "$maxDetectionFiles" ]; then
        filesToRemove=$((countFiles - maxDetectionFiles))
        find "$dir" -type f -printf '%T@ %p\n' 2>/dev/null | sort -n | head -n "$filesToRemove" | cut -d' ' -f2- | while IFS= read -r file; do
            rm -f -- "$file"
        done
        echo "$TIMESTAMP ${label}: removed $filesToRemove files, now $maxDetectionFiles" >> $LOGFILE
    fi

    # remove empty date folders left behind after the cleanup
    find "$dir" -depth -type d -empty -delete 2>/dev/null
}

clean_detection_dir "$MAINDIR/$ALTANKA_DET" "ALTANKA_DET"
clean_detection_dir "$MAINDIR/$BRAMA_DET" "BRAMA_DET"

# a lot of logs here. clear it daily
#truncate -s 0 /var/log/proftpd/vroot.log
#truncate -s 0 /var/log/auth.log
