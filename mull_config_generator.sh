#!/bin/bash

# This script builds a new config file for mull based on the arguments passed

OUTPUT="output/mull.yml"

# The script takes up to 5 arguments. If no arguments are passed, the script will print usage and set the timeout to 99999999 ms:
# 1. Use –mutators to pass in mutation operators you would like to enable 
# 2. Use –timeout flag to set the timeout 
# 3. Use –excludePaths to pass in paths to exclude separated by ; in quotation “” 
# 4. Use –includePaths to pass in paths to include separated by ; in quotation “” 
# 5. Use –quiet to pass in false to enable additional logging 

# Example usage: ./mull_config_generator.sh –mutators=cxx_add_to_sub –timeout=10 –excludePaths=”/Users/username/Desktop/mull_test/exclude” –includePaths=”/Users/username/Desktop/mull_test/include” –quiet=false
if [ $# -eq 0 ]; then
    echo "Usage: ./mull_config_generator.sh –mutators=cxx_add_to_sub –timeout=10 –excludePaths=”/Users/username/Desktop/mull_test/exclude” –includePaths=”/Users/username/Desktop/mull_test/include” –quiet=false"
    echo "Setting timeout to 99999999 ms"
    timeout="99999999"
fi

# make output directory if it doesn't exist in the current directory
mkdir -p output

# The script will generate a config file called mull.yml in the parent directory of the script
# The script will overwrite any existing mull.yml file in the parent directory of the script
for i in "$@"
do
case $i in
    -m=*|--mutators=*)
    mutators="${i#*=}"
    shift # past argument=value
    ;;
    -t=*|--timeout=*)
    timeout="${i#*=}"
    shift # past argument=value
    ;;
    -e=*|--excludePaths=*)
    excludePaths="${i#*=}"
    shift # past argument=value
    ;;
    -i=*|--includePaths=*)
    includePaths="${i#*=}"
    shift # past argument=value
    ;;
    -q=*|--quiet=*)
    quiet="${i#*=}"
    shift # past argument=value
    ;;
    *)
          # unknown option
    ;;
esac
done

# If no mutators are passed, the script will set the mutators to empty
if [ -z "$mutators" ]; then
    mutators=""
fi

# If no excludePaths are passed, the script will set the excludePaths to empty
if [ -z "$excludePaths" ]; then
    excludePaths=""
fi

# If no includePaths are passed, the script will set the includePaths to empty
if [ -z "$includePaths" ]; then
    includePaths=""
fi

# If no quiet is passed, the script will set the quiet to true
if [ -z "$quiet" ]; then
    quiet="true"
fi

# If no timeout is passed, the script will set the timeout to 99999999 ms
if [ -z "$timeout" ]; then
    timeout="99999999"
fi

echo "Generating mull.yml file in parent directory of script"
echo "timeout: $timeout" > $OUTPUT

# If mutators are passed, the script will iterate and add the mutators to the config file
if [ -n "$mutators" ]; then
    echo "mutators:" >> $OUTPUT
    IFS=';' read -ra ADDR <<< "$mutators"
    for i in "${ADDR[@]}"; do
        echo "  - $i" >> $OUTPUT
    done
fi

# If excludePaths are passed, the script will iterate and add the excludePaths to the config file
if [ -n "$excludePaths" ]; then
    echo "excludePaths:" >> $OUTPUT
    IFS=';' read -ra ADDR <<< "$excludePaths"
    for i in "${ADDR[@]}"; do
        echo "  - $i" >> $OUTPUT
    done
fi

# If includePaths are passed, the script will iterate and add the includePaths to the config file
if [ -n "$includePaths" ]; then
    echo "includePaths:" >> $OUTPUT
    IFS=';' read -ra ADDR <<< "$includePaths"
    for i in "${ADDR[@]}"; do
        echo "  - $i" >> $OUTPUT
    done
fi

# If quiet is passed, the script will add the quiet to the config file
if [ -n "$quiet" ]; then
    echo "quiet: $quiet" >> output/mull.yml
fi

echo "mull.yml file generated in parent directory of script"