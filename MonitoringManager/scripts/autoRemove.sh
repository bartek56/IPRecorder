#!/bin/bash

LOGFILE="/var/log/autoRemove.log"
MAINDIR="/srv/dev-disk-by-uuid-8c47c0f4-500d-404f-9b3f-b380f844edd1/MONITORING"
TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
maxCatalogs=10


echo "$TIMESTAMP start autoremove script" >> $LOGFILE

countDir=$(ls -d $MAINDIR/altanka_cam/*/ | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

if [ $countFilesToRemove -ge 1 ]; then

    catalogsToRemove=$(ls -d $MAINDIR/altanka_cam/*/ | head -${countFilesToRemove})
    for eachCatalog in $catalogsToRemove
    do
        rm -rf $eachCatalog
        echo "$TIMESTAMP ALTANKA: removed $eachCatalog" >> $LOGFILE
    done
fi

countDir=$(ls -d $MAINDIR/brama_cam/*/ | wc -l)
countFilesToRemove=$(($countDir-$maxCatalogs))

if [ $countFilesToRemove -ge 1 ]; then
    catalogsToRemove=$(ls -d $MAINDIR/brama_cam/*/ | head -${countFilesToRemove})
    for eachCatalog in $catalogsToRemove
    do
        rm -rf $eachCatalog
        echo "$TIMESTAMP BRAMA: removed $eachCatalog" >> $LOGFILE
    done
fi


TIMESTAMP=`date "+%Y-%m-%d %H:%M:%S"`
echo "$TIMESTAMP finished autoremove script" >> $LOGFILE

# a lot of logs here. clear it daily
#truncate -s 0 /var/log/proftpd/vroot.log
#truncate -s 0 /var/log/auth.log
