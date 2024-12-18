apply patch for micropython
	1. cd lib/micropython
	
	When there is an error in patching, execute this' git checkout ', Then apply the patch again
	2. git checkout d7d77d91becd0716ac1672c92652d9dd72ec613f -b adfmpy
	
	3. git apply ../../patchs4lib/micropython.patch
	
apply patch for esp-idf
	1. cd lib/esp-idf

	When there is an error in patching, execute this' git checkout ', Then apply the patch again
	2. git checkout 11eaf41b37267ad7709c0899c284e3683d2f0b5e -b adfmpy

	3. git apply ../../patchs4lib/esp-idf.patch


