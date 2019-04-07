#!/bin/bash
pushd `dirname "$0"`

FINALRESULT=0
SMC32=../bin/smc

# Clean output files
rm -f data/*.sf1 data/*.cf1

# Test case 1:
echo '---------------------------------------------------------------'
echo 'TEST CASE 1'
echo '---------------------------------------------------------------'
$SMC32 data/ca01.smc
$SMC32 --deterministic-output data/ca01.sf1 data/music8.mus
diff data/ca01.cf1 data/ca01.cf1.expected
DIFF_RESULT=$?
echo '---------------------------------------------------------------'
if [ $DIFF_RESULT -ne 0 ]; then
    echo -e '\e[31mFAILED\e[0m'
    FINALRESULT=1
else
    echo -e '\e[32mPASSED\e[0m'
fi
echo '---------------------------------------------------------------'

# Test case 2:
echo '---------------------------------------------------------------'
echo 'TEST CASE 2'
echo '---------------------------------------------------------------'
$SMC32 --deterministic-output data/ca10a.smc
diff data/ca10a.sf1 data/ca10a.sf1.expected
DIFF_RESULT=$?
echo '---------------------------------------------------------------'
if [ $DIFF_RESULT -ne 0 ]; then
    echo -e '\e[31mFAILED\e[0m'
    FINALRESULT=1
else
    echo -e '\e[32mPASSED\e[0m'
fi
echo '---------------------------------------------------------------'

# Test case 3:
echo '---------------------------------------------------------------'
echo 'TEST CASE 3'
echo '---------------------------------------------------------------'
$SMC32 --deterministic-output data/ba12aa.smc
diff data/ba12aa.sf1 data/ba12aa.sf1.expected
DIFF_RESULT=$?
echo '---------------------------------------------------------------'
if [ $DIFF_RESULT -ne 0 ]; then
    echo -e '\e[31mFAILED\e[0m'
    FINALRESULT=1
else
    echo -e '\e[32mPASSED\e[0m'
fi
echo '---------------------------------------------------------------'

# Test case 4:
echo '---------------------------------------------------------------'
echo 'TEST CASE 4'
echo '---------------------------------------------------------------'
$SMC32 --deterministic-output data/ca08jf.smc
diff data/ca08jf.sf1 data/ca08jf.sf1.expected
DIFF_RESULT=$?
echo '---------------------------------------------------------------'
if [ $DIFF_RESULT -ne 0 ]; then
    echo -e '\e[31mFAILED\e[0m'
    FINALRESULT=1
else
    echo -e '\e[32mPASSED\e[0m'
fi
echo '---------------------------------------------------------------'

popd
exit $FINALRESULT
