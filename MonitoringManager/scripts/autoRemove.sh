#!/bin/bash

LOGFILE="/var/log/autoRemove.log"
TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
maxCatalogs=10


echo "$TIMESTAMP start autoremove script" >> $LOGFILE




countDir=$(ls -d /sharedfolders/MONITORING/altanka_cam/*/ | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

catalogsToRemove=$(ls -d /sharedfolders/MONITORING/altanka_cam/*/ | head -${countFilesToRemove})
for eachCatalog in $catalogsToRemove
do
    rm -rf $eachCatalog
    echo "$TIMESTAMP ALTANKA: removed $eachCatalog" >> $LOGFILE
done


countDir=$(ls -d /sharedfolders/MONITORING/brama_cam/*/ | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

catalogsToRemove=$(ls -d /sharedfolders/MONITORING/brama_cam/*/ | head -${countFilesToRemove})
for eachCatalog in $catalogsToRemove
do
    rm -rf $eachCatalog
    echo "$TIMESTAMP BRAMA: removed $eachCatalog" >> $LOGFILE
done




TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
echo "$TIMESTAMP finished autoremove script" >> $LOGFILE

# a lot of logs here. clear it daily
truncate -s 0 /var/log/proftpd/vroot.log
truncate -s 0 /var/log/auth.log
