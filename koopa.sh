# Check running this script from the root of the repository.
if [[ ! -d "case" ]]; then
    echo "Please run this script from the root of the repository."
    exit 1
fi

# Get case file name from id.
id="${1}"
id="$(printf "%03d" "${id}")" # Add leading zeros.
case_file=""
for file in `ls case`; do
    if [[ "${file}" == "${id}"* ]]; then
        case_file="$file"
        break
    fi
done

# Check case file exists.
if [[ -z "${case_file}" ]]; then
    echo "Case file not found."
    exit 1
fi
echo "Use case file ${case_file}."

# Run case.
build/compiler -koopa case/${case_file} -o build/${id}

# Print output.
more build/${id}
