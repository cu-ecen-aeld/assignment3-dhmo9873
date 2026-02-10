#!/bin/bash
# Tester script for SINGLE-CONNECTION aesdsocket

pushd `dirname $0` > /dev/null

target=localhost
port=9000

function printusage
{
	echo "Usage: $0 [-t target_ip] [-p port]"
	echo "	Runs a single socket test on the aesdsocket application"
	echo "	target_ip defaults to ${target}"
	echo "	port defaults to ${port}"
}

while getopts "t:p:" opt; do
	case ${opt} in
		t ) target=$OPTARG ;;
		p ) port=$OPTARG ;;
		\? | : )
			printusage
			exit 1
			;;
	esac
done

echo "Testing target ${target} on port ${port}"

# Single test only (server exits after one connection)
string="abcdefg"

expected_file=$(mktemp)
new_file=$(mktemp)

echo "sending string ${string} to ${target} on port ${port}"
echo "${string}" | nc ${target} ${port} -w 1 > ${new_file}

echo "${string}" > ${expected_file}

diff ${expected_file} ${new_file} > /dev/null
if [ $? -ne 0 ]; then
	echo "Test FAILED"
	echo "Expected:"
	cat ${expected_file}
	echo "Received:"
	cat ${new_file}
	diff -u ${expected_file} ${new_file}
	rm ${expected_file} ${new_file}
	exit 1
fi

echo "Test PASSED"
echo "Received data:"
cat ${new_file}

rm ${expected_file} ${new_file}
echo "Single-connection test complete!"

popd > /dev/null

