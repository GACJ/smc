#!/bin/bash
pushd `dirname "$0"`

FINALRESULT=0
SMC32=../bin/smc32

# Clean output files
rm data/*.sf1 data/*.cf1

# Test case 1:
echo '---------------------------------------------------------------'
echo 'TEST CASE 1'
echo '---------------------------------------------------------------'
$SMC32 data/ca01.smc
$SMC32 data/ca01.sf1 data/music8.mus
diff <(tail -n +4 data/ca01.cf1) <(tail -n +4 data/ca01.cf1.expected) &>/dev/null
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
