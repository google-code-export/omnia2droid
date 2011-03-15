#!/bin/bash
# script to (simplyfy the) build a kernel_update
KERNEL_MODULES_PATH=~/omnia2droid/svn/trunk/out
if [ ! -d "${KERNEL_MODULES_PATH}" ]
then
	echo -e "\n ${KERNEL_MODULES_PATH} not found!\nplease correct the KERNEL_MODULES_PATH in ${0} !\n";
	exit 1;
fi

# let's try ... see if we have a revision-string
if [ $# -eq 1 ] 
then
	if [ ! -d "./o2b2update" ]
	then
		echo -e "./o2b2update not found ... "
		mkdir -m 777 ./o2b2update
		if [ $? -eq 0 ]
		then
			echo -e "created.\n"
		else
			echo -e "creation failed!"
			exit 1;
		fi
	fi
	
	cd ./o2b2update/
	
	if [ $? -eq 0 ]
	then
		
		# if local modules-dir not exists ...
		if [ ! -d "./modules" ]
		then
			# ... it will be created
			mkdir -m 777 ./modules
		else
			# else : remove older version modules (just to get sure)
			rm -f ./modules/*
		fi
		
		# remove older version zImage
		if [ -f ./zImage ]
		then
			rm -f ./zImage
		fi
		
		# remove older version o2b2update
		if [ -f ./o2b2update.tar.gz ]
		then
			rm -f ./o2b2update.tar.gz
		fi
		
		# copy kernel-modules
		cp ${KERNEL_MODULES_PATH}/*.ko ./modules/
		if [ $?  -gt 0 ]
		then
			 echo -e "\nERROR! no modules found in  ${KERNEL_MODULES_PATH}\n"
			 exit 1;
		else
			 echo "modules copied"
		fi
		
		# copy kernel
		cp ${KERNEL_MODULES_PATH}/zImage ./
		if [ $? -gt 0 ]
		then
			 echo -e "\nERROR! no zImage found in  ${KERNEL_MODULES_PATH}\n"
			 exit 1;
		else
			 echo "zImage copied"
		fi
		
		# create tar-ball from modules
		tar -czpf o2b2update.tar.gz ./modules
		if [ $? -eq 0 ]
		then
			# grant all permissions (just to get sure)
			chmod 777 o2b2update.tar.gz
			 echo "o2b2update.tar.gz created"
		else
			echo -e "\nERROR! while creating tar-ball (o2b2update.tar.gz)\n"
			exit 1;
		fi

		# create zip-file with new zImage & new o2b2upadte
		echo "try to create Kernel_Rev${1}.zip ..."
		zip ./Kernel_Rev${1}.zip ./o2b2update.tar.gz ./zImage
		if [ $? -gt 0 ]
		then
			 echo -e "\nERROR! while creating zip-file Kernel_Rev${1}.zip of o2b2update.tar.gz & zImage\n"
			 exit 1;
		else
			echo "file: Kernel_Rev${1}.zip created";
		fi
		
		mv  Kernel_Rev${1}.zip ../
		if [ $? -eq 0 ]
		then
			cd ../
			echo "file: Kernel_Rev${1}.zip contains modules (o2b2update.tar.gz) & zImage"
			exit 0;
		else
			echo -e "\nERROR! - seems that the file Kernel_Rev${1}.zip is not present (created)\n"
			exit 1;
		fi
	else
		echo "could not change into directory ./o2b2update";
		exit 1;
	fi
else
 		
 		echo -e "Usage: ${0} <REV>\n\te.g.: ${0} 93"	
		exit 1; 		
fi
